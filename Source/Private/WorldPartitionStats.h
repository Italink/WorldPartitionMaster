#pragma once

#include "WorldPartitionStats.generated.h"

USTRUCT()
struct FWorldPartitionTextureStats {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString Path;

	UPROPERTY(VisibleAnywhere)
	int MemorySize;

	UPROPERTY(VisibleAnywhere)
	FIntPoint TextureSize;
};

USTRUCT()
struct FWorldPartitionActorStats {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FTopLevelAssetPath BaseClass;

	UPROPERTY(VisibleAnywhere)
	FTopLevelAssetPath NativeClass;

	UPROPERTY(VisibleAnywhere)
	FSoftObjectPath Path;

	UPROPERTY(VisibleAnywhere)
	FName Package;

	UPROPERTY(VisibleAnywhere)
	FString Label;

	UPROPERTY(VisibleAnywhere)
	FGuid ActorGuid;

	UPROPERTY(VisibleAnywhere)
	int DrawCallCount;

	UPROPERTY(VisibleAnywhere)
	int TriangleCount;

	UPROPERTY(VisibleAnywhere)
	int TextureCount;
};

USTRUCT()
struct FWorldPartitionHlodStats {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FWorldPartitionActorStats HlodActor;
};

USTRUCT()
struct FWorldPartitionCellStats {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName CellName;

	UPROPERTY(VisibleAnywhere)
	FName CellPackage;

	UPROPERTY(VisibleAnywhere)
	FBox Bounds;

	UPROPERTY(VisibleAnywhere)
	int HierarchicalLevel = 0;

	UPROPERTY(VisibleAnywhere)
	int Priority = 0;

	UPROPERTY(VisibleAnywhere)
	bool bIsSpatiallyLoaded = 0;

	UPROPERTY(VisibleAnywhere)
	TArray<FName> DataLayers;

	UPROPERTY(VisibleAnywhere)
	TArray<FWorldPartitionActorStats> Actors;

	UPROPERTY(VisibleAnywhere)
	FWorldPartitionHlodStats Hlod;

	UPROPERTY(VisibleAnywhere)
	int DrawCallCount = 0;

	UPROPERTY(VisibleAnywhere)
	int TriangleCount = 0;

	UPROPERTY(VisibleAnywhere)
	TMap<FString, int> ComponentCount;
	TArray<FSoftObjectPath> UsedTextures;
};

USTRUCT()
struct FWorldPartitionGridStats {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName GridName;

	UPROPERTY(VisibleAnywhere)
	FBox Bounds;

	UPROPERTY(VisibleAnywhere)
	int32 CellSize = 0;

	UPROPERTY(VisibleAnywhere)
	int32 LoadingRange = 0;

	UPROPERTY(VisibleAnywhere)
	int MaxHierarchicalLevel = 0;

	UPROPERTY(VisibleAnywhere)
	TArray<FWorldPartitionCellStats> Cells;
};

USTRUCT()
struct FWorldPartitionStats {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FWorldPartitionGridStats> Grids;

	UPROPERTY(VisibleAnywhere)
	int TotalTriangles;

	static FWorldPartitionStats FlushStreaming(UWorld* InWorld);
};

UENUM()
enum class EWorldPartitionStatsPreviewMode {
	None,
	Cell,
	Actor
};

UCLASS()
class UWorldPartitionStatsCellPreviewer : public UObject{
    GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere)
	FWorldPartitionCellStats CellStats;
};