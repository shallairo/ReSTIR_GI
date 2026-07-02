#include "ReSTIRGIRuntimeModule.h"

#include "Interfaces/IPluginManager.h"
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

	ViewExtension = FSceneViewExtensions::NewExtension<FReSTIRGIViewExtension>();
}

void FReSTIRGIRuntimeModule::ShutdownModule()
{
	ViewExtension.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FReSTIRGIRuntimeModule, ReSTIRGIRuntime)
