#include "WorldPartitionStats.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "WorldPartition/WorldPartitionStreamingDescriptor.h"
#include "WorldPartition/WorldPartitionRuntimeHash.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "WorldPartition/HLOD/HLODActor.h"
#include "WorldPartition/HLOD/HLODActorDesc.h"

FWorldPartitionStats FWorldPartitionStats::FlushStreaming(UWorld* InWorld)
{
	FWorldPartitionStats Stats;
	if (InWorld == nullptr || InWorld->GetWorldPartition() == nullptr)
		return Stats;
	UWorldPartition* WorldPartition = InWorld->GetWorldPartition();

	UE::Private::WorldPartition::FStreamingDescriptor StreamingDesc;
	UE::Private::WorldPartition::FStreamingDescriptor::FStreamingDescriptorParams Params;
	UE::Private::WorldPartition::FStreamingDescriptor::GenerateStreamingDescriptor(InWorld, StreamingDesc, Params);

	for (UE::Private::WorldPartition::FStreamingDescriptor::FStreamingGrid& Grid : StreamingDesc.StreamingGrids) {
		FWorldPartitionGridStats& GridStats = Stats.Grids.AddDefaulted_GetRef();
		GridStats.GridName = Grid.Name;
		GridStats.Bounds = Grid.Bounds;
		GridStats.CellSize = Grid.CellSize;
		GridStats.LoadingRange = Grid.LoadingRange;
		for (UE::Private::WorldPartition::FStreamingDescriptor::FStreamingCell& Cell : Grid.StreamingCells) {
			FWorldPartitionCellStats& CellStats = GridStats.Cells.AddDefaulted_GetRef();
			CellStats.CellPackage = Cell.CellPackage;
			CellStats.Bounds = Cell.Bounds;
			CellStats.bIsSpatiallyLoaded = Cell.bIsSpatiallyLoaded;
			CellStats.DataLayers = Cell.DataLayers;
			for (const UE::Private::WorldPartition::FStreamingDescriptor::FStreamingActor& Actor : Cell.Actors) {
				FWorldPartitionHandle ActorHandle(WorldPartition, Actor.ActorGuid);
				if (ActorHandle.ContainerInstance == nullptr || ActorHandle.ActorDescInstance == nullptr)
					continue;
				FWorldPartitionActorStats ActorStats;
				ActorStats.BaseClass = Actor.BaseClass;
				ActorStats.NativeClass = Actor.NativeClass;
				ActorStats.Path = Actor.Path;
				ActorStats.Package = Actor.Package;
				ActorStats.Label = Actor.Label;
				ActorStats.ActorGuid = Actor.ActorGuid;
				if (ActorStats.Label.IsEmpty()) {
					ActorStats.Label = ActorHandle->GetActorLabelString();
				}
				CellStats.Actors.Add(ActorStats);
			}
		}
	}

	UWorldPartition::FGenerateStreamingParams StreamingParams = UWorldPartition::FGenerateStreamingParams();
	UWorldPartition::FGenerateStreamingContext Context;
	WorldPartition->GenerateStreaming(StreamingParams, Context);

	FWorldPartitionGridStats& GridStats = Stats.Grids.AddDefaulted_GetRef();
	GridStats.GridName = "EditorAlwaysLoadedActors";
	FWorldPartitionCellStats& CellStats = GridStats.Cells.AddDefaulted_GetRef();
	CellStats.CellName = "EditorAlwaysLoaded";
	for (TActorIterator<AActor> It(InWorld, AActor::StaticClass()); It; ++It){
		AActor* Actor = *It;
		if (Actor && !Actor->GetIsSpatiallyLoaded()) {
			FWorldPartitionActorStats& ActorStats = CellStats.Actors.AddDefaulted_GetRef();
			ActorStats.ActorGuid = Actor->GetActorGuid();
			ActorStats.Label = Actor->GetActorLabel();
			ActorStats.Package = *Actor->GetPathName();
			ActorStats.Path = Actor->GetPathName();
		}
	}

	TMap<FGuid, FWorldPartitionHlodStats> HlodStatsMap;

	WorldPartition->ForEachActorDescContainerInstance([&](UActorDescContainerInstance* ActorDescContainerInstance) {
		for (UActorDescContainerInstance::TConstIterator<AWorldPartitionHLOD> HLODIterator(ActorDescContainerInstance); HLODIterator; ++HLODIterator) {
			const FHLODActorDesc& HLODActorDesc = *(FHLODActorDesc*)HLODIterator->GetActorDesc();
			AWorldPartitionHLOD* HLODActor = Cast<AWorldPartitionHLOD>(HLODIterator->GetActor());
			if (HLODActor) {
				FWorldPartitionHlodStats HlodStats;
				HlodStats.HlodActor.ActorGuid = HLODActor->GetActorGuid();
				HlodStats.HlodActor.Label = HLODActor->GetActorLabel();
				HlodStats.HlodActor.Package = *HLODActor->GetPathName();
				HlodStats.HlodActor.Path = HLODActor->GetPathName();
				HlodStats.HlodActor.BaseClass = HLODActor->GetClass()->GetPathName();
				HlodStatsMap.Add(HLODActor->GetSourceCellGuid(), HlodStats);
			}
		}
	});

	WorldPartition->RuntimeHash->ForEachStreamingCells([&Stats,&HlodStatsMap](const UWorldPartitionRuntimeCell* Cell) {
		FWorldPartitionGridStats* GridStats = Stats.Grids.FindByPredicate([Cell](const FWorldPartitionGridStats& GridStats) {
			return GridStats.GridName == Cell->RuntimeCellData->GridName;
		});
		if (GridStats == nullptr) {
			return true;
		}
		FWorldPartitionCellStats* CellStats = GridStats->Cells.FindByPredicate([Cell, GridStats](const FWorldPartitionCellStats& CellStats) {
			return CellStats.CellPackage.ToString().Contains(Cell->GetName());
		});
		if (CellStats == nullptr) {
			return true;
		}
		CellStats->CellName = *Cell->GetDebugName();
		CellStats->HierarchicalLevel = Cell->RuntimeCellData->HierarchicalLevel;
		CellStats->Priority = Cell->RuntimeCellData->Priority;
		if (HlodStatsMap.Contains(Cell->GetGuid())) {
			CellStats->Hlod = HlodStatsMap[Cell->GetGuid()];
		}

		GridStats->HierarchicalCellCount.FindOrAdd(CellStats->HierarchicalLevel)++;

		for (auto& ActorStats : CellStats->Actors) {
			AActor* Actor = Cast<AActor>(ActorStats.Path.TryLoad());
			if (ActorStats.Label.IsEmpty()) {
				ActorStats.Label = Actor->GetActorLabel();
			}
			if (ActorStats.Path.IsNull()) {
				ActorStats.Path = ActorStats.Path;
			}
			TArray<UActorComponent*> ActorCompoents;
			TSet<UTexture*> CellUsedTextures;
			Actor->GetComponents(ActorCompoents, true);
			for (auto ActorComp : ActorCompoents) {
				CellStats->ComponentCount.FindOrAdd(ActorComp->GetClass()->GetName())++;
				if (auto SMC = Cast<UStaticMeshComponent>(ActorComp)) {
					UStaticMesh* Mesh = SMC->GetStaticMesh();
					if (Mesh == nullptr)
						continue;
					ActorStats.DrawCallCount += Mesh->GetNumSections(0);
					if (auto ISMC = Cast<UInstancedStaticMeshComponent>(SMC)) {
						ActorStats.TriangleCount += Mesh->GetNumTriangles(0) * ISMC->GetInstanceCount();
					}
					else {
						ActorStats.TriangleCount += Mesh->GetNumTriangles(0);
					}
					for (UMaterialInterface* Material : SMC->GetMaterials()) {
						if (Material) {
							TArray<UTexture*> MaterialTextures;
							Material->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::Num, true);
							CellUsedTextures.Append(MaterialTextures);
						}
					}
				}
			}
			CellStats->DrawCallCount += ActorStats.DrawCallCount;
			CellStats->TriangleCount += ActorStats.TriangleCount;
			CellStats->UsedTextures.Append(CellUsedTextures.Array());
		}
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Cell->GetDebugName());
		return true;
	});
	WorldPartition->FlushStreaming();
	return Stats;
}

UWorld* UWorldPartitionStatsCellPreviewer::GetWorld() const
{
	return World;
}
