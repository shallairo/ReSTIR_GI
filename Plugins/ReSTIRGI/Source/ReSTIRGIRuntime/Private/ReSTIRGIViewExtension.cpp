#include "ReSTIRGIViewExtension.h"

#include "PostProcess/PostProcessMaterialInputs.h"
#include "ReSTIRGISettings.h"
#include "ReSTIRGIShaders.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScreenPass.h"
#include "FXRenderingUtils.h"
#include "GPUProfiler.h"
#include "HAL/IConsoleManager.h"

DECLARE_GPU_STAT_NAMED(ReSTIRGI, TEXT("ReSTIR GI"));

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
		TEXT("Overrides ReSTIR GI debug view. -1 panel."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarReSTIRGIRadianceMode(
		TEXT("r.ReSTIRGI.RadianceMode"),
		-1,
		TEXT("Overrides ReSTIR GI secondary radiance mode. -1 panel, 0 screen-projected, 1 synthetic constant, 2 normal color, 3 albedo color."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarReSTIRGIIntensity(
		TEXT("r.ReSTIRGI.Intensity"),
		-1.0f,
		TEXT("Overrides ReSTIR GI composite intensity. -1 uses the editor panel setting."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarReSTIRGIMaxRayDistance(
		TEXT("r.ReSTIRGI.MaxRayDistance"),
		-1.0f,
		TEXT("Overrides ReSTIR GI maximum secondary ray distance in world units. -1 uses the editor panel setting."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarReSTIRGINormalBias(
		TEXT("r.ReSTIRGI.NormalBias"),
		-1.0f,
		TEXT("Overrides ReSTIR GI ray normal bias in world units. -1 uses the editor panel setting."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarReSTIRGIHalfResolution(
		TEXT("r.ReSTIRGI.HalfResolution"),
		-1,
		TEXT("Overrides ReSTIR GI internal reservoir resolution. -1 panel, 0 full resolution, 1 half resolution."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarReSTIRGIConservativeVisibility(
		TEXT("r.ReSTIRGI.ConservativeVisibility"),
		-1,
		TEXT("Overrides ReSTIR GI conservative/final visibility. -1 panel, 0 off, 1 on."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarReSTIRGIBoilingFilterStrength(
		TEXT("r.ReSTIRGI.BoilingFilterStrength"),
		-1.0f,
		TEXT("Overrides ReSTIR GI boiling filter strength. -1 uses the editor panel setting."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarReSTIRGILumenCompareMode(
		TEXT("r.ReSTIRGI.LumenCompareMode"),
		-1,
		TEXT("Overrides ReSTIR GI Lumen compare mode. -1 panel, 0 overlay, 1 ReSTIR only, 2 Lumen only, 3 wipe, 4 side-by-side."),
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
			Settings.DebugView = static_cast<EReSTIRGIDebugView>(FMath::Clamp(DebugOverride, 0, static_cast<int32>(EReSTIRGIDebugView::IndirectRaw)));
		}

		const int32 RadianceOverride = CVarReSTIRGIRadianceMode.GetValueOnRenderThread();
		if (RadianceOverride >= 0)
		{
			Settings.RadianceMode = static_cast<EReSTIRGIRadianceMode>(FMath::Clamp(RadianceOverride, 0, 3));
		}

		const float IntensityOverride = CVarReSTIRGIIntensity.GetValueOnRenderThread();
		if (IntensityOverride >= 0.0f)
		{
			Settings.GIIntensity = IntensityOverride;
		}

		const float MaxRayDistanceOverride = CVarReSTIRGIMaxRayDistance.GetValueOnRenderThread();
		if (MaxRayDistanceOverride >= 0.0f)
		{
			Settings.MaxRayDistance = MaxRayDistanceOverride;
		}

		const float NormalBiasOverride = CVarReSTIRGINormalBias.GetValueOnRenderThread();
		if (NormalBiasOverride >= 0.0f)
		{
			Settings.NormalBias = NormalBiasOverride;
		}

		const int32 HalfResolutionOverride = CVarReSTIRGIHalfResolution.GetValueOnRenderThread();
		if (HalfResolutionOverride >= 0)
		{
			Settings.bHalfResolution = HalfResolutionOverride != 0;
		}

		const int32 ConservativeVisibilityOverride = CVarReSTIRGIConservativeVisibility.GetValueOnRenderThread();
		if (ConservativeVisibilityOverride >= 0)
		{
			Settings.bConservativeVisibility = ConservativeVisibilityOverride != 0;
		}

		const float BoilingFilterOverride = CVarReSTIRGIBoilingFilterStrength.GetValueOnRenderThread();
		if (BoilingFilterOverride >= 0.0f)
		{
			Settings.BoilingFilterStrength = BoilingFilterOverride;
		}

		const int32 LumenCompareOverride = CVarReSTIRGILumenCompareMode.GetValueOnRenderThread();
		if (LumenCompareOverride >= 0)
		{
			Settings.LumenCompareMode = static_cast<EReSTIRGILumenCompareMode>(FMath::Clamp(LumenCompareOverride, 0, 4));
		}
	}

	FReSTIRGICommonShaderParameters MakeCommonParameters(
		const FReSTIRGISettings& Settings,
		const FIntPoint& ViewRectMin,
		const FIntPoint& ViewRectSize,
		const FIntPoint& BufferSize,
		const FIntPoint& ReservoirSize,
		const bool bHistoryValid,
		const bool bRayTracingAvailable)
	{
		FReSTIRGICommonShaderParameters Parameters;
		Parameters.ViewRectMin = ViewRectMin;
		Parameters.ViewRectSize = ViewRectSize;
		Parameters.BufferSize = BufferSize;
		Parameters.ReservoirSize = ReservoirSize;
		Parameters.FrameIndex = GFrameNumber;
		Parameters.ResamplingMode = static_cast<uint32>(Settings.ResamplingMode);
		Parameters.DebugView = static_cast<uint32>(Settings.DebugView);
		Parameters.RadianceMode = static_cast<uint32>(Settings.RadianceMode);
		Parameters.SpatialSampleCount = FMath::Clamp(Settings.SpatialSampleCount, 1, 32);
		Parameters.MaxHistoryLength = FMath::Clamp(Settings.MaxHistoryLength, 1, 255);
		Parameters.SpatialRadius = FMath::Max(Settings.SpatialRadius, 1.0f);
		Parameters.NormalThreshold = FMath::Clamp(Settings.NormalThreshold, 0.0f, 1.0f);
		Parameters.DepthThreshold = FMath::Max(Settings.DepthThreshold, 0.0001f);
		Parameters.RoughnessMISClamp = FMath::Clamp(Settings.RoughnessMISClamp, 0.02f, 1.0f);
		Parameters.RadianceClamp = FMath::Max(Settings.RadianceClamp, 0.0f);
		Parameters.GIIntensity = FMath::Clamp(Settings.GIIntensity, 0.0f, 20.0f);
		Parameters.CompareSplit = FMath::Clamp(Settings.CompareSplit, 0.0f, 1.0f);
		Parameters.MaxRayDistance = FMath::Max(Settings.MaxRayDistance, 1.0f);
		Parameters.NormalBias = FMath::Max(Settings.NormalBias, 0.0f);
		Parameters.DiffuseRayProbability = FMath::Clamp(Settings.DiffuseRayProbability, 0.0f, 1.0f);
		Parameters.SecondaryRoughnessClamp = FMath::Clamp(Settings.SecondaryRoughnessClamp, 0.02f, 1.0f);
		Parameters.BoilingFilterStrength = FMath::Clamp(Settings.BoilingFilterStrength, 0.0f, 1.0f);
		Parameters.SyntheticConstantIntensity = FMath::Clamp(Settings.SyntheticConstantIntensity, 0.0f, 10.0f);
		Parameters.bFinalMIS = Settings.bFinalMIS ? 1u : 0u;
		Parameters.bFinalVisibility = Settings.bFinalVisibility ? 1u : 0u;
		Parameters.bHistoryValid = bHistoryValid ? 1u : 0u;
		Parameters.bRayTracingAvailable = bRayTracingAvailable ? 1u : 0u;
		Parameters.bConservativeVisibility = Settings.bConservativeVisibility ? 1u : 0u;
		Parameters.LumenCompareMode = static_cast<uint32>(Settings.LumenCompareMode);
		return Parameters;
	}

	void ApplyLumenCompareMode(const EReSTIRGILumenCompareMode CompareMode)
	{
		static bool bStoredPreviousValues = false;
		static int32 PreviousDiffuseIndirectAllow = 1;
		static int32 PreviousReflectionsAllow = 1;

		IConsoleVariable* LumenDiffuse = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.DiffuseIndirect.Allow"));
		IConsoleVariable* LumenReflections = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.Allow"));
		if (!LumenDiffuse || !LumenReflections)
		{
			return;
		}

		if (!bStoredPreviousValues)
		{
			PreviousDiffuseIndirectAllow = LumenDiffuse->GetInt();
			PreviousReflectionsAllow = LumenReflections->GetInt();
			bStoredPreviousValues = true;
		}

		if (CompareMode == EReSTIRGILumenCompareMode::ReSTIROnly)
		{
			LumenDiffuse->Set(0, ECVF_SetByConsole);
			LumenReflections->Set(0, ECVF_SetByConsole);
		}
		else
		{
			LumenDiffuse->Set(PreviousDiffuseIndirectAllow, ECVF_SetByConsole);
			LumenReflections->Set(PreviousReflectionsAllow, ECVF_SetByConsole);
		}
	}

	EReSTIRGILumenCompareMode GetEffectiveLumenCompareMode_GameThread()
	{
		FReSTIRGISettings Settings = FReSTIRGISettingsManager::GetSettings();
		if (!Settings.bEnabled)
		{
			return EReSTIRGILumenCompareMode::Overlay;
		}

		const int32 LumenCompareOverride = CVarReSTIRGILumenCompareMode.GetValueOnGameThread();
		if (LumenCompareOverride >= 0)
		{
			return static_cast<EReSTIRGILumenCompareMode>(FMath::Clamp(LumenCompareOverride, 0, 4));
		}

		return Settings.LumenCompareMode;
	}

	FRDGBufferRef CreateReservoirBuffer(FRDGBuilder& GraphBuilder, const TCHAR* Name, const int32 ReservoirCount)
	{
		return GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FReSTIRGIPackedReservoir), ReservoirCount),
			Name);
	}

	FRDGBufferRef CreateSurfaceBuffer(FRDGBuilder& GraphBuilder, const TCHAR* Name, const int32 SurfaceCount)
	{
		return GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FReSTIRGISurface), SurfaceCount),
			Name);
	}

	FRDGBufferRef CreateStatsBuffer(FRDGBuilder& GraphBuilder)
	{
		// 4 slots: 0=initial, 1=temporal, 2=spatial, 3=final
		return GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FReSTIRGIStats), 4),
			TEXT("ReSTIRGI.StatsBuffer"));
	}

	FRDGBufferRef CreateNeighborOffsetBuffer(FRDGBuilder& GraphBuilder)
	{
		static const uint32 EncodedOffsets[] =
		{
			0x00010000u, 0xffff0000u, 0x00000001u, 0x0000ffffu,
			0x00010001u, 0xffff0001u, 0x0001ffffu, 0xffffffffu,
			0x00020000u, 0xfffe0000u, 0x00000002u, 0x0000fffeu,
			0x00020001u, 0xfffe0001u, 0x0001fffeu, 0xffffffffu,
			0x00020002u, 0xfffe0002u, 0x0002fffeu, 0xfffefffeu,
			0x00030000u, 0xfffd0000u, 0x00000003u, 0x0000fffdu,
			0x00030001u, 0xfffd0001u, 0x0001fffdu, 0xffffffffu,
			0x00030002u, 0xfffd0002u, 0x0002fffdu, 0xfffefffdu,
			0x00030003u, 0xfffd0003u, 0x0003fffdu, 0xfffdfffdu,
			0x00040000u, 0xfffc0000u, 0x00000004u, 0x0000fffcu,
			0x00040001u, 0xfffc0001u, 0x0001fffcu, 0xffffffffu,
			0x00040002u, 0xfffc0002u, 0x0002fffcu, 0xfffefffcu,
			0x00040003u, 0xfffc0003u, 0x0003fffcu, 0xfffdfffcu,
			0x00040004u, 0xfffc0004u, 0x0004fffcu, 0xfffcfffcu
		};

		FRDGBufferRef Buffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), UE_ARRAY_COUNT(EncodedOffsets)),
			TEXT("ReSTIRGI.NeighborOffsetBuffer"));
		GraphBuilder.QueueBufferUpload(Buffer, EncodedOffsets, sizeof(EncodedOffsets));
		return Buffer;
	}

	void ClearStatsBuffer(FRDGBuilder& GraphBuilder, FRDGBufferRef StatsBuffer)
	{
		// Zero 4 slots (initial/temporal/spatial/final)
		const uint32 ZeroData[16] = {};
		GraphBuilder.QueueBufferUpload(StatsBuffer, ZeroData, sizeof(ZeroData));
	}
}

FReSTIRGIViewExtension::FReSTIRGIViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

void FReSTIRGIViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	const FReSTIRGISettings Settings = FReSTIRGISettingsManager::GetSettings();
	if (Settings.bEnabled
		&& Settings.ResamplingMode != EReSTIRGIResamplingMode::Off
		&& Settings.LumenCompareMode == EReSTIRGILumenCompareMode::ReSTIROnly)
	{
		InView.FinalPostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Plugin;
	}
}

void FReSTIRGIViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (IsInRenderingThread())
	{
		ProcessStatsReadback();
	}

	if (IsInGameThread())
	{
		ApplyLumenCompareMode(GetEffectiveLumenCompareMode_GameThread());
	}

	FReSTIRGIRuntimeStats Stats = FReSTIRGISettingsManager::GetStats();
	Stats.bViewExtensionActive = true;
	FReSTIRGISettingsManager::UpdateStats(Stats);
}

ESceneViewExtensionFlags FReSTIRGIViewExtension::GetFlags() const
{
	const FReSTIRGISettings Settings = FReSTIRGISettingsManager::GetSettings();
	if (Settings.bEnabled
		&& Settings.ResamplingMode != EReSTIRGIResamplingMode::Off
		&& Settings.LumenCompareMode != EReSTIRGILumenCompareMode::LumenOnly)
	{
		return ESceneViewExtensionFlags::RequiresHardwareInlineRayTracing;
	}

	return ESceneViewExtensionFlags::None;
}

bool FReSTIRGIViewExtension::ShouldApplyAtPass(EPostProcessingPass Pass, bool bIsPassEnabled) const
{
	return Pass == EPostProcessingPass::Tonemap;
}

void FReSTIRGIViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (ShouldApplyAtPass(Pass, bIsPassEnabled))
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FReSTIRGIViewExtension::AddReSTIRGIPass_RenderThread));
	}
}

void FReSTIRGIViewExtension::ResetHistory_RenderThread()
{
	HistoryReservoir.SafeRelease();
	HistoryPrimarySurfaceBuffer.SafeRelease();
	HistoryViewSize = FIntPoint::ZeroValue;
	HistoryReservoirSize = FIntPoint::ZeroValue;
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
	Stats.CurrentLumenMode = Settings.LumenCompareMode;

	if (!SceneColor.IsValid() || !Settings.bEnabled || Settings.ResamplingMode == EReSTIRGIResamplingMode::Off || Settings.LumenCompareMode == EReSTIRGILumenCompareMode::LumenOnly)
	{
		Stats.ViewSize = SceneColor.IsValid() ? SceneColor.ViewRect.Size() : FIntPoint::ZeroValue;
		Stats.bHistoryValid = bHistoryValid;
		Stats.PipelineStatus = Settings.LumenCompareMode == EReSTIRGILumenCompareMode::LumenOnly
			? TEXT("Lumen Only 模式：ReSTIR pass 已按预期跳过")
			: TEXT("ReSTIR GI 未启用或算法模式为关闭");
		FReSTIRGISettingsManager::UpdateStats(Stats);
		return SceneColor;
	}

	if (Settings.bResetHistory)
	{
		ResetHistory_RenderThread();
		Settings.bResetHistory = false;
		FReSTIRGISettingsManager::UpdateSettings(Settings);
	}

	const FIntPoint ViewRectMin = SceneColor.ViewRect.Min;
	const FIntPoint ViewRectSize = SceneColor.ViewRect.Size();
	const FIntPoint BufferSize = FIntPoint(SceneColor.Texture->Desc.GetSize().X, SceneColor.Texture->Desc.GetSize().Y);
	const FIntPoint ReservoirSize = Settings.bHalfResolution
		? FIntPoint(FMath::DivideAndRoundUp(ViewRectSize.X, 2), FMath::DivideAndRoundUp(ViewRectSize.Y, 2))
		: ViewRectSize;
	const int32 ReservoirCount = FMath::Max(1, ReservoirSize.X * ReservoirSize.Y);
	const bool bSizeChanged = HistoryViewSize != ViewRectSize || HistoryReservoirSize != ReservoirSize;
	if (bSizeChanged)
	{
		ResetHistory_RenderThread();
	}

	FRDGBufferSRVRef RayTracingSceneView = nullptr;
#if RHI_RAYTRACING
	if (View.Family && View.Family->Scene && UE::FXRenderingUtils::RayTracing::HasRayTracingScene(*View.Family->Scene))
	{
		RayTracingSceneView = UE::FXRenderingUtils::RayTracing::GetRayTracingSceneViewRDG(*View.Family->Scene, View);
	}
#endif

	const bool bRayTracingAvailable = RayTracingSceneView != nullptr;
	if (!Inputs.SceneTextures.SceneTextures || !bRayTracingAvailable)
	{
		Stats.ViewSize = ViewRectSize;
		Stats.ReservoirSize = ReservoirSize;
		Stats.bHistoryValid = false;
		Stats.bRayTracingAvailable = bRayTracingAvailable;
		Stats.PipelineStatus = !Inputs.SceneTextures.SceneTextures
			? TEXT("SceneTextures 不可用：无法读取 GBuffer/SceneColor")
			: TEXT("TLAS 不可用：等待 UE 构建 RayTracingScene，请确认 DX12/SM6/硬件光追与 ReSTIRGI RT delegate");
		FReSTIRGISettingsManager::UpdateStats(Stats);
		return SceneColor;
	}

	RDG_EVENT_SCOPE_STAT(GraphBuilder, ReSTIRGI, "ReSTIRGI");

	if (LastLumenCompareMode != Settings.LumenCompareMode)
	{
		ResetHistory_RenderThread();
		LastLumenCompareMode = Settings.LumenCompareMode;
	}

	const FReSTIRGICommonShaderParameters CommonParameters = MakeCommonParameters(
		Settings, ViewRectMin, ViewRectSize, BufferSize, ReservoirSize, bHistoryValid, bRayTracingAvailable);
	const FIntVector ReservoirGroupCount = FComputeShaderUtils::GetGroupCount(ReservoirSize, FIntPoint(GReSTIRGIThreadGroupSize, GReSTIRGIThreadGroupSize));
	const FIntVector OutputGroupCount = FComputeShaderUtils::GetGroupCount(ViewRectSize, FIntPoint(GReSTIRGIThreadGroupSize, GReSTIRGIThreadGroupSize));
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());

	FRDGBufferRef PrimarySurfaceBuffer = CreateSurfaceBuffer(GraphBuilder, TEXT("ReSTIRGI.PrimarySurfaceBuffer"), ReservoirCount);
	FRDGBufferRef SecondarySurfaceBuffer = CreateSurfaceBuffer(GraphBuilder, TEXT("ReSTIRGI.SecondarySurfaceBuffer"), ReservoirCount);
	FRDGBufferRef InitialReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.InitialReservoir"), ReservoirCount);
	FRDGBufferRef TemporalReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.TemporalReservoir"), ReservoirCount);
	FRDGBufferRef SpatialReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.SpatialReservoir"), ReservoirCount);
	FRDGBufferRef HistoryOutputReservoir = CreateReservoirBuffer(GraphBuilder, TEXT("ReSTIRGI.HistoryReservoirOutput"), ReservoirCount);
	FRDGBufferRef HistoryPrimarySurfaceOutput = CreateSurfaceBuffer(GraphBuilder, TEXT("ReSTIRGI.HistoryPrimarySurfaceOutput"), ReservoirCount);
	FRDGBufferRef NeighborOffsetBuffer = CreateNeighborOffsetBuffer(GraphBuilder);
	FRDGBufferRef StatsBuffer = CreateStatsBuffer(GraphBuilder);
	ClearStatsBuffer(GraphBuilder, StatsBuffer);

	{
		FReSTIRGIInitialSampleCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGIInitialSampleCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = Inputs.SceneTextures.SceneTextures;
		PassParameters->TLAS = RayTracingSceneView;
		PassParameters->SceneColorTexture = SceneColor.Texture;
		PassParameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->PrimarySurfaceBuffer = GraphBuilder.CreateUAV(PrimarySurfaceBuffer);
		PassParameters->SecondarySurfaceBuffer = GraphBuilder.CreateUAV(SecondarySurfaceBuffer);
		PassParameters->InitialReservoir = GraphBuilder.CreateUAV(InitialReservoir);
		PassParameters->StatsBuffer = GraphBuilder.CreateUAV(StatsBuffer);

		TShaderMapRef<FReSTIRGIInitialSampleCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Initial Sample"), Shader, PassParameters, ReservoirGroupCount);
	}

	const bool bNeedsTemporalPass = Settings.ResamplingMode == EReSTIRGIResamplingMode::Temporal
		|| Settings.ResamplingMode == EReSTIRGIResamplingMode::TemporalAndSpatial;
	FRDGBufferRef FinalSourceReservoir = InitialReservoir;

	if (bNeedsTemporalPass)
	{
		FRDGBufferRef HistoryInputReservoir = HistoryReservoir.IsValid()
			? GraphBuilder.RegisterExternalBuffer(HistoryReservoir, TEXT("ReSTIRGI.HistoryReservoir"))
			: InitialReservoir;
		FRDGBufferRef HistoryInputPrimarySurface = HistoryPrimarySurfaceBuffer.IsValid()
			? GraphBuilder.RegisterExternalBuffer(HistoryPrimarySurfaceBuffer, TEXT("ReSTIRGI.HistoryPrimarySurface"))
			: PrimarySurfaceBuffer;

		FReSTIRGITemporalResamplingCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGITemporalResamplingCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->PrimarySurfaceBufferSRV = GraphBuilder.CreateSRV(PrimarySurfaceBuffer);
		PassParameters->HistoryPrimarySurfaceBuffer = GraphBuilder.CreateSRV(HistoryInputPrimarySurface);
		PassParameters->InitialReservoir = GraphBuilder.CreateSRV(InitialReservoir);
		PassParameters->HistoryReservoir = GraphBuilder.CreateSRV(HistoryInputReservoir);
		PassParameters->TemporalReservoir = GraphBuilder.CreateUAV(TemporalReservoir);
		PassParameters->StatsBuffer = GraphBuilder.CreateUAV(StatsBuffer);

		TShaderMapRef<FReSTIRGITemporalResamplingCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Temporal Resampling"), Shader, PassParameters, ReservoirGroupCount);
		FinalSourceReservoir = TemporalReservoir;
	}

	if (Settings.ResamplingMode == EReSTIRGIResamplingMode::InitialOnly)
	{
		FinalSourceReservoir = InitialReservoir;
	}
	else if (Settings.ResamplingMode == EReSTIRGIResamplingMode::Spatial || Settings.ResamplingMode == EReSTIRGIResamplingMode::TemporalAndSpatial)
	{
		FReSTIRGISpatialResamplingCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGISpatialResamplingCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->PrimarySurfaceBufferSRV = GraphBuilder.CreateSRV(PrimarySurfaceBuffer);
		PassParameters->NeighborOffsetBuffer = GraphBuilder.CreateSRV(NeighborOffsetBuffer);
		PassParameters->SourceReservoir = GraphBuilder.CreateSRV(Settings.ResamplingMode == EReSTIRGIResamplingMode::Spatial ? InitialReservoir : TemporalReservoir);
		PassParameters->SpatialReservoir = GraphBuilder.CreateUAV(SpatialReservoir);
		PassParameters->StatsBuffer = GraphBuilder.CreateUAV(StatsBuffer);

		TShaderMapRef<FReSTIRGISpatialResamplingCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Spatial Resampling"), Shader, PassParameters, ReservoirGroupCount);
		FinalSourceReservoir = SpatialReservoir;
	}

	FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
	OutputDesc.Flags |= TexCreate_UAV | TexCreate_ShaderResource;
	FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("ReSTIRGI.CompositeOutput"));

	{
		FReSTIRGIFinalCompositeCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReSTIRGIFinalCompositeCS::FParameters>();
		PassParameters->Common = CommonParameters;
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = Inputs.SceneTextures.SceneTextures;
		PassParameters->TLAS = RayTracingSceneView;
		PassParameters->SceneColorTexture = SceneColor.Texture;
		PassParameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->PrimarySurfaceBufferSRV = GraphBuilder.CreateSRV(PrimarySurfaceBuffer);
		PassParameters->SecondarySurfaceBufferSRV = GraphBuilder.CreateSRV(SecondarySurfaceBuffer);
		PassParameters->FinalReservoir = GraphBuilder.CreateSRV(FinalSourceReservoir);
		PassParameters->InitialReservoir = GraphBuilder.CreateUAV(InitialReservoir);
		PassParameters->TemporalReservoir = GraphBuilder.CreateUAV(TemporalReservoir);
		PassParameters->SpatialReservoir = GraphBuilder.CreateUAV(SpatialReservoir);
		PassParameters->HistoryPrimarySurfaceOutput = GraphBuilder.CreateUAV(HistoryPrimarySurfaceOutput);
		PassParameters->HistoryReservoirOutput = GraphBuilder.CreateUAV(HistoryOutputReservoir);
		PassParameters->OutputTexture = GraphBuilder.CreateUAV(OutputTexture);
		PassParameters->StatsBuffer = GraphBuilder.CreateUAV(StatsBuffer);

		TShaderMapRef<FReSTIRGIFinalCompositeCS> Shader(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ReSTIRGI Final Composite"), Shader, PassParameters, OutputGroupCount);
	}

	if (!Settings.bFreezeHistory)
	{
		GraphBuilder.QueueBufferExtraction(HistoryOutputReservoir, &HistoryReservoir);
		GraphBuilder.QueueBufferExtraction(HistoryPrimarySurfaceOutput, &HistoryPrimarySurfaceBuffer);
		HistoryViewSize = ViewRectSize;
		HistoryReservoirSize = ReservoirSize;
		bHistoryValid = true;
	}

	// Extract stats buffer for readback
	GraphBuilder.QueueBufferExtraction(StatsBuffer, &StatsReadbackBuffer);

	Stats.ViewSize = ViewRectSize;
	Stats.ReservoirSize = ReservoirSize;
	Stats.ReservoirBytes = static_cast<uint64>(ReservoirCount) * (sizeof(FReSTIRGIPackedReservoir) * 4ull + sizeof(FReSTIRGISurface) * 3ull);
	Stats.EstimatedRaysPerPixel = 1;
	Stats.bHistoryValid = bHistoryValid;
	Stats.bRayTracingAvailable = bRayTracingAvailable;
	Stats.PipelineStatus = TEXT("ReSTIR GI pass 已调度：Initial/Temporal/Spatial/Final 可在 ProfileGPU 中查看");
	FReSTIRGISettingsManager::UpdateStats(Stats);

	return FScreenPassTexture(OutputTexture, SceneColor.ViewRect);
}

void FReSTIRGIViewExtension::ProcessStatsReadback()
{
	// Release the previous frame's stats buffer. GPU readback via TryLock is not
	// directly available on FRDGPooledBuffer; stats ratios remain at -1.0 (N/A)
	// in the UI until a proper RHIGPUBufferReadback integration is added.
	StatsReadbackBuffer.SafeRelease();
}
