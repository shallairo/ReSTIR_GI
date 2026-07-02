#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;

class FReSTIRGIEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnDebugTab(const FSpawnTabArgs& Args);
	void RegisterMenus();
	void OpenDebugTab();

	static const FName DebugTabName;
};
