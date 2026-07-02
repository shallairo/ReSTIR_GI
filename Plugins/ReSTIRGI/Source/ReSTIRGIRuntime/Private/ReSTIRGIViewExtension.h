#pragma once

#include "CoreMinimal.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphResources.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"

struct FPostProcessMaterialInputs;

class FReSTIRGIViewExtension final : public FSceneViewExtensionBase
{
public:
	explicit FReSTIRGIViewExtension(const FAutoRegister& AutoRegister);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	FScreenPassTexture AddReSTIRGIPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	void ResetHistory_RenderThread();

	TRefCountPtr<FRDGPooledBuffer> HistoryReservoir;
	FIntPoint HistoryViewSize = FIntPoint::ZeroValue;
	bool bHistoryValid = false;
};
