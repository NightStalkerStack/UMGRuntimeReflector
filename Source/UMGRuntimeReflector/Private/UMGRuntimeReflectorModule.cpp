#include "UMGRuntimeReflectorModule.h"

#include "SUMGRuntimeReflector.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FUMGRuntimeReflectorModule"

const FName FUMGRuntimeReflectorModule::ReflectorTabName(TEXT("UMGRuntimeReflector"));

void FUMGRuntimeReflectorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ReflectorTabName,
		FOnSpawnTab::CreateRaw(this, &FUMGRuntimeReflectorModule::SpawnReflectorTab))
		.SetDisplayName(LOCTEXT("ReflectorTabTitle", "UMG运行时反射器"))
		.SetTooltipText(LOCTEXT("ReflectorTabTooltip", "观察PIE运行时的UUserWidget与UWidget树"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUMGRuntimeReflectorModule::RegisterMenus));
}

void FUMGRuntimeReflectorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnregisterOwner(this);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ReflectorTabName);
}

TSharedRef<SDockTab> FUMGRuntimeReflectorModule::SpawnReflectorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SUMGRuntimeReflector)
		];
}

void FUMGRuntimeReflectorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("WindowLayout"));
	Section.AddMenuEntry(
		TEXT("OpenUMGRuntimeReflector"),
		LOCTEXT("OpenReflectorLabel", "UMG运行时反射器"),
		LOCTEXT("OpenReflectorTooltip", "打开UMG运行时反射器"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FUMGRuntimeReflectorModule::OpenReflectorTab)));
}

void FUMGRuntimeReflectorModule::OpenReflectorTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ReflectorTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUMGRuntimeReflectorModule, UMGRuntimeReflector)
