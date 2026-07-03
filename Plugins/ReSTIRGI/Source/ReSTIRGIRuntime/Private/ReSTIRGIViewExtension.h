#pragma once

#include "CoreMinimal.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ReSTIRGISettings.h"
#include "RHIGPUReadback.h"
#include "RenderGraphResources.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"

struct FPostProcessMaterialInputs;

class FReSTIRGIViewExtension final : public FSceneViewExtensionBase
{
public:
	explicit FReSTIRGIViewExtension(const FAutoRegister& AutoRegister);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual ESceneViewExtensionFlags GetFlags() const override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	FScreenPassTexture AddReSTIRGIPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	bool ShouldApplyAtPass(EPostProcessingPass Pass, bool bIsPassEnabled) const;
	void ResetHistory_RenderThread();
	void ProcessStatsReadback();

	TRefCountPtr<FRDGPooledBuffer> HistoryReservoir;
	TRefCountPtr<FRDGPooledBuffer> HistoryPrimarySurfaceBuffer;
	TUniquePtr<FRHIGPUBufferReadback> StatsReadbackBuffer;
	FIntPoint HistoryViewSize = FIntPoint::ZeroValue;
	FIntPoint HistoryReservoirSize = FIntPoint::ZeroValue;
	EReSTIRGILumenCompareMode LastLumenCompareMode = EReSTIRGILumenCompareMode::Overlay;
	bool bHistoryValid = false;
	bool bWasInPIE = false;
};
