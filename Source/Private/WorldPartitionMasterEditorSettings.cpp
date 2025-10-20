#include "WorldPartitionMasterEditorSettings.h"

UWorldPartitionMasterEditorSettings* UWorldPartitionMasterEditorSettings::Get()
{
	return GetMutableDefault<UWorldPartitionMasterEditorSettings>();
}

FWorldPartitionStats UWorldPartitionMasterEditorSettings::FindOrGetStats(FName MapName)
{
	if (mMapStats.Contains(MapName)) {
		return mMapStats[MapName];
	}
	return FWorldPartitionStats();
}

void UWorldPartitionMasterEditorSettings::SetStats(FName MapName, FWorldPartitionStats Stats)
{
	mMapStats.Add(MapName, Stats);
	TryUpdateDefaultConfigFile();
}

