#pragma once

#include "Modules/ModuleManager.h"

class FWorldPartitionMasterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
protected:
	static TSharedRef<class SDockTab> SpawnWorldPartitionMaster(const FSpawnTabArgs& Args);

protected:
};
