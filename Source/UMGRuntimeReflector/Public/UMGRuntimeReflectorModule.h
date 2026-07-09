#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUMGRuntimeReflectorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<class SDockTab> SpawnReflectorTab(const class FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();
	void OpenReflectorTab();

private:
	static const FName ReflectorTabName;
};
