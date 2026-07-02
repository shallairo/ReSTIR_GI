#include "ReSTIRGIEditorModule.h"

#include "Framework/Docking/TabManager.h"
#include "SReSTIRGIDebugPanel.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FReSTIRGIEditorModule"

const FName FReSTIRGIEditorModule::DebugTabName(TEXT("ReSTIRGIDebugPanel"));

void FReSTIRGIEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DebugTabName,
		FOnSpawnTab::CreateRaw(this, &FReSTIRGIEditorModule::SpawnDebugTab))
		.SetDisplayName(LOCTEXT("ReSTIRGIDebugTabTitle", "ReSTIR GI Debug"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FReSTIRGIEditorModule::RegisterMenus));
}

void FReSTIRGIEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DebugTabName);
}

TSharedRef<SDockTab> FReSTIRGIEditorModule::SpawnDebugTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SReSTIRGIDebugPanel)
		];
}

void FReSTIRGIEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("WindowLayout"));

	Section.AddMenuEntry(
		TEXT("OpenReSTIRGIDebugPanel"),
		LOCTEXT("OpenReSTIRGIDebugPanel", "ReSTIR GI Debug"),
		LOCTEXT("OpenReSTIRGIDebugPanelTooltip", "Open the ReSTIR GI realtime tuning and comparison panel."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FReSTIRGIEditorModule::OpenDebugTab)));
}

void FReSTIRGIEditorModule::OpenDebugTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DebugTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FReSTIRGIEditorModule, ReSTIRGIEditor)
