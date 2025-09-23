#pragma once

struct FWorldPartitionTextureStats {
	FString Path;
	int MemorySize;
	FIntPoint TextureSize;
};

struct FWorldPartitionActorStats {
	FTopLevelAssetPath BaseClass;
	FTopLevelAssetPath NativeClass;
	FSoftObjectPath Path;
	FName Package;
	FString Label;
	FGuid ActorGuid;

	int DrawCallCount;
	int TriangleCount;
	int TextureCount;
};

struct FWorldPartitionHlodStats {
	FWorldPartitionActorStats HlodActor;
};

struct FWorldPartitionCellStats {
	FName CellName;
	FName CellPackage;
	FBox Bounds;
	int HierarchicalLevel = 0;
	int Priority = 0;
	bool bIsSpatiallyLoaded = 0;
	TArray<FName> DataLayers;
	TArray<TSharedPtr<FWorldPartitionActorStats>> Actors;
	TSharedPtr<FWorldPartitionHlodStats> Hlod;
	int DrawCallCount;
	int TriangleCount;
	TMap<FString, int> ComponentCount;
	TArray<FSoftObjectPath> UsedTextures;
};

struct FWorldPartitionGridStats {
	FName GridName;
	FBox Bounds;
	int32 CellSize = 0;
	int32 LoadingRange = 0;
	int MaxHierarchicalLevel = 0;
	TArray<TSharedPtr<FWorldPartitionCellStats>> Cells;
};

struct FWorldPartitionStats {
	TArray<TSharedPtr<FWorldPartitionGridStats>> Grids;
	int TotalTriangles;
	static TSharedPtr<FWorldPartitionStats> FlushStreaming(UWorld* InWorld);
};