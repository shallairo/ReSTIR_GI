#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FReSTIRGIViewExtension;

class FReSTIRGIRuntimeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterViewExtension();
	void RegisterRayTracingDelegates();
	void UnregisterRayTracingDelegates();
	void OnAnyRayTracingPassEnabled(bool& bAnyRayTracingPassEnabled);

	TSharedPtr<FReSTIRGIViewExtension, ESPMode::ThreadSafe> ViewExtension;
	FDelegateHandle PostEngineInitHandle;
	FDelegateHandle AnyRayTracingPassEnabledHandle;
};
