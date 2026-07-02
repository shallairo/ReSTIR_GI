#include "ReSTIRGIViewExtension.h"

#include "PostProcess/PostProcessMaterialInputs.h"
#include "ReSTIRGISettings.h"
#include "ReSTIRGIShaders.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScreenPass.h"
#include "HAL/IConsoleManager.h"

namespace
{
	constexpr int32 GReSTIRGIThreadGroupSize = 8;

	TAutoConsoleVariable<int32> CVarReSTIRGIEnabled(
		TEXT("r.ReSTIRGI.Enabled"),
		-1,
		TEXT("Overrides the ReSTIR GI enabled state. -1 uses the editor panel setting, 0 disables, 1 enables."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarReSTIRGIMode(
		TEXT("r.ReSTIRGI.Mode"),
		-1,
		TEXT("Overrides ReSTIR GI mode. -1 panel, 0 off, 1 initial, 2 temporal, 3 spatial, 4 temporal+spatial."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarReSTIRGIDebugView(
		TEXT("r.ReSTIRGI.DebugView"),
		-1,
		TEXT("Overrides ReSTIR GI debug view. -1 panel, 0 composite, 4 weight, 5 age, 6 heatmap, 7 wipe, 8 side-by-side."),
		ECVF_RenderThreadSafe);

	void ApplyConsoleOverrides(FReSTIRGISettings& Settings)
	{
		const int32 EnabledOverride = CVarReSTIRGIEnabled.GetValueOnRenderThread();
		if (EnabledOverride >= 0)
		{
			Settings.bEnabled = EnabledOverride != 0;
		}

		const int32 ModeOverride = CVarReSTIRGIMode.GetValueOnRenderThread();
		if (ModeOverride >= 0)
		{
			Settings.ResamplingMode = static_cast<EReSTIRGIResamplingMode>(FMath::Clamp(ModeOverride, 0, 4));
		}

		const int32 DebugOverride = CVarReSTIRGIDebugView.GetValueOnRenderThread();
		if (DebugOverride >= 0)
		{
			Settings.DebugView = static_cast<EReSTIRGIDebugView>(FMath::Clamp(DebugOverride, 0, 8));
		}
	}

	FReSTIRGICommonShaderParameters MakeCommonParameters(const FReSTIRGISettings& Settings, const FIntPoint ViewSize, const bool bHistoryValid)
	{
		FReSTIRGICommonShaderParameters Parameters;
		Parameters.ViewSize = ViewSize;
		Parameters.FrameIndex = GFrameNumber;
		Parameters.ResamplingMode = static_cast<uint32>(Settings.ResamplingMode);
		Parameters.DebugView = static_cast<uint32>(Settings.DebugView);
		Parameters.SpatialSampleCount = FMath::Clamp(Settings.SpatialSampleCount, 1, 32);
		Parameters.MaxHistoryLength = FMath::Clamp(Settings.MaxHistoryLength, 1, 255);
		Parameters.SpatialRadius = FMath::Max(Settings.SpatialRadius, 1.0f);
		Parameters.NormalThreshold = FMath::Clamp(Settings.NormalThreshold, 0.0f, 1.0f);
		Parameters.DepthThreshold = FMath::Max(Settings.DepthThreshold, 0.0001f);
		Parameters.RoughnessMISClamp = FMath::Clamp(Settings.RoughnessMISClamp, 0.02f, 1.0f);
		Parameters.RadianceClamp = FMath::Max(Settings.RadianceClamp, 0.0f);
		Parameters.CompareSplit = FMath::Clamp(Settings.CompareSplit, 0.0f, 1.0f);
		Parameters.bFinalMIS = Settings.bFinalMIS ? 1u : 0u;
		Parameters.bFinalVisibility = Settings.bFinalVisibility ? 1u : 0u;
		Parameters.bHistoryValid = bHistoryValid ? 1u : 0u;
		return Parameters;
	}

	FRDGBufferRef CreateReservoirBuffer(FRDGBuilder& GraphBuilder, const TCHAR* Name, const int32 ReservoirCount)
	{
		return GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FReSTIRGIPackedReservoir), ReservoirCount),
			Name);
	}

	FRDGBufferRef CreateHistoryFallbackBuffer(FRDGBuilder& GraphBuilder, const int32 ReservoirCount)
	{
		return CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.EmptyHistoryReservoir"), ReservoirCount);
	}
}

FReSTIRGIViewExtension::FReSTIRGIViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

void FReSTIRGIViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FReSTIRGIViewExtension::AddReSTIRGIPass_RenderThread));
	}
}

void FReSTIRGIViewExtension::ResetHistory_RenderThread()
{
	HistoryReservoir.SafeRelease();
	HistoryViewSize = FIntPoint::ZeroValue;
	bHistoryValid = false;
}

FScreenPassTexture FReSTIRGIViewExtension::AddReSTIRGIPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	check(IsInRenderingThread());

	FReSTIRGISettings Settings = FReSTIRGISettingsManager::GetSettings();
	ApplyConsoleOverrides(Settings);
	FScreenPassTexture SceneColor(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

	FReSTIRGIRuntimeStats Stats;
	Stats.bViewExtensionActive = true;

	if (!SceneColor.IsValid() || !Settings.bEnabled || Settings.ResamplingMode == EReSTIRGIResamplingMode::Off)
	{
		Stats.ViewSize = SceneColor.IsValid() ? SceneColor.ViewRect.Size() : FIntPoint::ZeroValue;
		Stats.bHistoryValid = bHistoryValid;
		FReSTIRGISettingsManager::UpdateStats(Stats);
		return SceneColor;
	}

	if (Settings.bResetHistory)
	{
		ResetHistory_RenderThread();
		Settings.bResetHistory = false;
		FReSTIRGISettingsManager::UpdateSettings(Settings);
	}

	const FIntPoint ViewSize = SceneColor.ViewRect.Size();
	const int32 ReservoirCount = FMath::Max(1, ViewSize.X * ViewSize.Y);
	const bool bSizeChanged = HistoryViewSize != ViewSize;
	if (bSizeChanged)
	{
		ResetHistory_RenderThread();
	}

	const FReSTIRGICommonShaderParameters CommonParameters = MakeCommonParameters(Settings, ViewSize, bHistoryValid);
	const FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(ViewSize, FIntPoint(GReSTIRGIThreadGroupSize, GReSTIRGIThreadGroupSize));
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());

	FRDGBufferRef InitialReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.InitialReservoir"), ReservoirCount);
	FRDGBufferRef TemporalReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.TemporalReservoir"), ReservoirCount);
	FRDGBufferRef SpatialReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.SpatialReservoir"), ReservoirCount);
	FRDGBufferRef HistoryInputReservoir = HistoryReservoir.IsValid()
		? GraphBuilder.RegisterExternalBuffer(HistoryReservoir, TEXT("ReSTIRGI.HistoryReservoir"))
		: CreateHistoryFallbackBuffer(GraphBuilder, ReservoirCount);
	FRDGBufferRef HistoryOutputReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.HistoryReservoirOutput"), ReservoirCount);

	{
		FReSTIRGIInitialSampleCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGIInitialSampleCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->SceneColorTexture = SceneColor.Texture;
		PassParameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->InitialReservoir = GraphBuilder.CreateUAV(InitialReservoir);

		TShaderMapRef<FReSTIRGIInitialSampleCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Initial Sample"), Shader, PassParameters, GroupCount);
	}

	{
		FReSTIRGITemporalResamplingCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGITemporalResamplingCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->InitialReservoir = GraphBuilder.CreateSRV(InitialReservoir);
		PassParameters->HistoryReservoir = GraphBuilder.CreateSRV(HistoryInputReservoir);
		PassParameters->TemporalReservoir = GraphBuilder.CreateUAV(TemporalReservoir);

		TShaderMapRef<FReSTIRGITemporalResamplingCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Temporal Resampling"), Shader, PassParameters, GroupCount);
	}

	FRDGBufferRef FinalSourceReservoir = TemporalReservoir;
	if (Settings.ResamplingMode == EReSTIRGIResamplingMode::InitialOnly)
	{
		FinalSourceReservoir = InitialReservoir;
	}
	else if (Settings.ResamplingMode == EReSTIRGIResamplingMode::Spatial || Settings.ResamplingMode == EReSTIRGIResamplingMode::TemporalAndSpatial)
	{
		FReSTIRGISpatialResamplingCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGISpatialResamplingCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->SourceReservoir = GraphBuilder.CreateSRV(Settings.ResamplingMode == EReSTIRGIResamplingMode::Spatial ? InitialReservoir : TemporalReservoir);
		PassParameters->SpatialReservoir = GraphBuilder.CreateUAV(SpatialReservoir);

		TShaderMapRef<FReSTIRGISpatialResamplingCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Spatial Resampling"), Shader, PassParameters, GroupCount);
		FinalSourceReservoir = SpatialReservoir;
	}

	FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
	OutputDesc.Flags |= TexCreate_UAV | TexCreate_ShaderResource;
	FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("ReSTIRGI.CompositeOutput"));

	{
		FReSTIRGIFinalCompositeCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGIFinalCompositeCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->SceneColorTexture = SceneColor.Texture;
		PassParameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->FinalReservoir = GraphBuilder.CreateSRV(FinalSourceReservoir);
		PassParameters->HistoryReservoirOutput = GraphBuilder.CreateUAV(HistoryOutputReservoir);
		PassParameters->OutputTexture = GraphBuilder.CreateUAV(OutputTexture);

		TShaderMapRef<FReSTIRGIFinalCompositeCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Final Composite"), Shader, PassParameters, GroupCount);
	}

	if (!Settings.bFreezeHistory)
	{
		GraphBuilder.QueueBufferExtraction(HistoryOutputReservoir, &HistoryReservoir);
		HistoryViewSize = ViewSize;
		bHistoryValid = true;
	}

	Stats.ViewSize = ViewSize;
	Stats.ReservoirBytes = static_cast<uint64>(ReservoirCount) * sizeof(FReSTIRGIPackedReservoir) * 4ull;
	Stats.EstimatedRaysPerPixel = 1;
	Stats.bHistoryValid = bHistoryValid;
	FReSTIRGISettingsManager::UpdateStats(Stats);

	return FScreenPassTexture(OutputTexture, SceneColor.ViewRect);
}
