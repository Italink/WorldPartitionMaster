#pragma once

#include "WorldPartitionStats.h"
#include "WorldPartitionMasterEditorSettings.generated.h"

UCLASS(EditInlineNew, CollapseCategories, config = WorldPartitionMasterEditor)
class UWorldPartitionMasterEditorSettings : public UObject {
	GENERATED_BODY()
public:
	static UWorldPartitionMasterEditorSettings* Get();

	FWorldPartitionStats FindOrGetStats(FName MapName);
	void SetStats(FName MapName, FWorldPartitionStats Stats);

private:
	UPROPERTY(Config)
	TMap<FName, FWorldPartitionStats> mMapStats;
}; 
