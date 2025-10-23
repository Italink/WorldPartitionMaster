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
	FString Label;

	UPROPERTY()
	FSoftObjectPath Path;

	UPROPERTY(Transient, VisibleAnywhere)
	TSoftObjectPtr<AActor> Actor;

	UPROPERTY(VisibleAnywhere)
	FTopLevelAssetPath BaseClass;

	UPROPERTY(VisibleAnywhere)
	FTopLevelAssetPath NativeClass;

	UPROPERTY(VisibleAnywhere)
	FName Package;

	UPROPERTY(VisibleAnywhere)
	FGuid ActorGuid;

	UPROPERTY(VisibleAnywhere)
	int DrawCallCount = 0;

	UPROPERTY(VisibleAnywhere)
	int TriangleCount = 0;

	UPROPERTY(VisibleAnywhere)
	int TextureCount = 0;
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
	TMap<int, int> HierarchicalCellCount;

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
	UWorld* World = nullptr;
	virtual UWorld* GetWorld() const override;

	UPROPERTY(VisibleAnywhere)
	FWorldPartitionCellStats CellStats;
};