#include "ReSTIRGIRuntimeModule.h"

#include "DeferredShadingRenderer.h"
#include "Engine/Engine.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "ReSTIRGISettings.h"
#include "ReSTIRGIViewExtension.h"
#include "SceneViewExtension.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FReSTIRGIRuntimeModule"

void FReSTIRGIRuntimeModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ReSTIRGI"));
	if (Plugin.IsValid())
	{
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/ReSTIRGI"), ShaderDirectory);
	}

	RegisterRayTracingDelegates();

	if (GEngine)
	{
		RegisterViewExtension();
	}
	else
	{
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FReSTIRGIRuntimeModule::RegisterViewExtension);
	}
}

void FReSTIRGIRuntimeModule::ShutdownModule()
{
	UnregisterRayTracingDelegates();

	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	ViewExtension.Reset();
}

void FReSTIRGIRuntimeModule::RegisterRayTracingDelegates()
{
#if RHI_RAYTRACING
	if (!AnyRayTracingPassEnabledHandle.IsValid())
	{
		AnyRayTracingPassEnabledHandle = FGlobalIlluminationPluginDelegates::AnyRayTracingPassEnabled().AddRaw(this, &FReSTIRGIRuntimeModule::OnAnyRayTracingPassEnabled);
	}
#endif
}

void FReSTIRGIRuntimeModule::UnregisterRayTracingDelegates()
{
#if RHI_RAYTRACING
	if (AnyRayTracingPassEnabledHandle.IsValid())
	{
		FGlobalIlluminationPluginDelegates::AnyRayTracingPassEnabled().Remove(AnyRayTracingPassEnabledHandle);
		AnyRayTracingPassEnabledHandle.Reset();
	}
#endif
}

void FReSTIRGIRuntimeModule::OnAnyRayTracingPassEnabled(bool& bAnyRayTracingPassEnabled)
{
	const FReSTIRGISettings Settings = FReSTIRGISettingsManager::GetSettings();
	const bool bNeedsRayTracingScene = Settings.bEnabled
		&& Settings.ResamplingMode != EReSTIRGIResamplingMode::Off
		&& Settings.LumenCompareMode != EReSTIRGILumenCompareMode::LumenOnly;

	bAnyRayTracingPassEnabled = bAnyRayTracingPassEnabled || bNeedsRayTracingScene;
}

void FReSTIRGIRuntimeModule::RegisterViewExtension()
{
	if (!ViewExtension.IsValid() && GEngine)
	{
		ViewExtension = FSceneViewExtensions::NewExtension<FReSTIRGIViewExtension>();
		UE_LOG(LogTemp, Display, TEXT("ReSTIRGI: Scene view extension registered."));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FReSTIRGIRuntimeModule, ReSTIRGIRuntime)
