#include "WorldPartitionStats.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "WorldPartition/WorldPartitionStreamingDescriptor.h"
#include "WorldPartition/WorldPartitionRuntimeHash.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "WorldPartition/HLOD/HLODActor.h"
#include "WorldPartition/HLOD/HLODActorDesc.h"

TSharedPtr<FWorldPartitionStats> FWorldPartitionStats::FlushStreaming(UWorld* InWorld)
{
	TSharedPtr<FWorldPartitionStats> Stats = MakeShared<FWorldPartitionStats>();
	if (InWorld == nullptr || InWorld->GetWorldPartition() == nullptr)
		return Stats;
	UWorldPartition* WorldPartition = InWorld->GetWorldPartition();

	UE::Private::WorldPartition::FStreamingDescriptor StreamingDesc;
	UE::Private::WorldPartition::FStreamingDescriptor::FStreamingDescriptorParams Params;
	UE::Private::WorldPartition::FStreamingDescriptor::GenerateStreamingDescriptor(InWorld, StreamingDesc, Params);


	for (UE::Private::WorldPartition::FStreamingDescriptor::FStreamingGrid& Grid : StreamingDesc.StreamingGrids) {
		TSharedPtr<FWorldPartitionGridStats> GridStats = Stats->Grids.Add_GetRef(MakeShared<FWorldPartitionGridStats>());
		GridStats->GridName = Grid.Name;
		GridStats->Bounds = Grid.Bounds;
		GridStats->CellSize = Grid.CellSize;
		GridStats->LoadingRange = Grid.LoadingRange;
		for (UE::Private::WorldPartition::FStreamingDescriptor::FStreamingCell& Cell : Grid.StreamingCells) {
			TSharedPtr<FWorldPartitionCellStats> CellStats = GridStats->Cells.Add_GetRef(MakeShared<FWorldPartitionCellStats>());
			CellStats->CellPackage = Cell.CellPackage;
			CellStats->Bounds = Cell.Bounds;
			CellStats->bIsSpatiallyLoaded = Cell.bIsSpatiallyLoaded;
			CellStats->DataLayers = Cell.DataLayers;
			for (const UE::Private::WorldPartition::FStreamingDescriptor::FStreamingActor& Actor : Cell.Actors) {
				TSharedPtr<FWorldPartitionActorStats> ActorStats = CellStats->Actors.Add_GetRef(MakeShared<FWorldPartitionActorStats>());
				FWorldPartitionHandle ActorHandle(WorldPartition, Actor.ActorGuid);
				ActorStats->BaseClass = Actor.BaseClass;
				ActorStats->NativeClass = Actor.NativeClass;
				ActorStats->Path = Actor.Path;
				ActorStats->Package = Actor.Package;
				ActorStats->Label = Actor.Label;
				ActorStats->ActorGuid = Actor.ActorGuid;
				if (ActorStats->Label.IsEmpty()) {
					ActorStats->Label = ActorHandle->GetActorLabelString();
				}
			}
		}
	}
	UWorldPartition::FGenerateStreamingParams StreamingParams = UWorldPartition::FGenerateStreamingParams();
	UWorldPartition::FGenerateStreamingContext Context;
	WorldPartition->GenerateStreaming(StreamingParams, Context);

	TSharedPtr<FWorldPartitionGridStats> GridStats = Stats->Grids.Add_GetRef(MakeShared<FWorldPartitionGridStats>());
	GridStats->GridName = "EditorAlwaysLoadedActors";
	TSharedPtr<FWorldPartitionCellStats> CellStats = GridStats->Cells.Add_GetRef(MakeShared<FWorldPartitionCellStats>());
	CellStats->CellName = "EditorAlwaysLoaded";
	for (TActorIterator<AActor> It(InWorld, AActor::StaticClass()); It; ++It){
		AActor* Actor = *It;
		if (Actor && !Actor->GetIsSpatiallyLoaded()) {
			TSharedPtr<FWorldPartitionActorStats> ActorStats = CellStats->Actors.Add_GetRef(MakeShared<FWorldPartitionActorStats>());
			ActorStats->ActorGuid = Actor->GetActorGuid();
			ActorStats->Label = Actor->GetActorLabel();
			ActorStats->Package = *Actor->GetPathName();
		}
	}

	TMap<FGuid, TSharedPtr<FWorldPartitionHlodStats>> HlodStatsMap;

	WorldPartition->ForEachActorDescContainerInstance([&](UActorDescContainerInstance* ActorDescContainerInstance) {
		for (UActorDescContainerInstance::TConstIterator<AWorldPartitionHLOD> HLODIterator(ActorDescContainerInstance); HLODIterator; ++HLODIterator) {
			const FHLODActorDesc& HLODActorDesc = *(FHLODActorDesc*)HLODIterator->GetActorDesc();
			AWorldPartitionHLOD* HLODActor = Cast<AWorldPartitionHLOD>(HLODIterator->GetActor());
			if (HLODActor) {
				TSharedPtr<FWorldPartitionHlodStats> HlodStats = MakeShared<FWorldPartitionHlodStats>();
				HlodStats->HlodActor.ActorGuid = HLODActor->GetActorGuid();
				HlodStats->HlodActor.Label = HLODActor->GetActorLabel();
				HlodStats->HlodActor.Package = *HLODActor->GetPathName();
				HlodStats->HlodActor.Path = HLODActor->GetPathName();
				HlodStats->HlodActor.BaseClass = HLODActor->GetClass()->GetPathName();
				HlodStatsMap.Add(HLODActor->GetSourceCellGuid(), HlodStats);
			}
		}
	});

	WorldPartition->RuntimeHash->ForEachStreamingCells([&Stats,&HlodStatsMap](const UWorldPartitionRuntimeCell* Cell) {
		TSharedPtr<FWorldPartitionGridStats>* GridStats = Stats->Grids.FindByPredicate([Cell](const TSharedPtr<FWorldPartitionGridStats>& GridStats) {
			return GridStats->GridName == Cell->RuntimeCellData->GridName;
		});
		if (GridStats == nullptr) {
			return true;
		}
		TSharedPtr<FWorldPartitionCellStats>* CellStats = (*GridStats)->Cells.FindByPredicate([Cell, GridStats](const TSharedPtr<FWorldPartitionCellStats>& CellStats) {
			return CellStats->CellPackage.ToString().Contains(Cell->GetName());
		});
		if (CellStats == nullptr) {
			return true;
		}
		(*CellStats)->CellName = *Cell->GetDebugName();
		(*CellStats)->HierarchicalLevel = Cell->RuntimeCellData->HierarchicalLevel;
		(*CellStats)->Priority = Cell->RuntimeCellData->Priority;
		if (HlodStatsMap.Contains(Cell->GetGuid())) {
			(*CellStats)->Hlod = HlodStatsMap[Cell->GetGuid()];
		}
		(*GridStats)->MaxHierarchicalLevel = FMath::Max((*GridStats)->MaxHierarchicalLevel, (*CellStats)->HierarchicalLevel);

		for (auto& ActorStats : (*CellStats)->Actors) {
			AActor* Actor = Cast<AActor>(ActorStats->Path.TryLoad());
			if (ActorStats->Label.IsEmpty()) {
				ActorStats->Label = Actor->GetActorLabel();
			}
			TArray<UActorComponent*> ActorCompoents;
			TSet<UTexture*> CellUsedTextures;
			Actor->GetComponents(ActorCompoents, true);
			for (auto ActorComp : ActorCompoents) {
				(*CellStats)->ComponentCount.FindOrAdd(ActorComp->GetClass()->GetName())++;
				if (auto SMC = Cast<UStaticMeshComponent>(ActorComp)) {
					UStaticMesh* Mesh = SMC->GetStaticMesh();
					ActorStats->DrawCallCount += Mesh->GetNumSections(0);
					if (auto ISMC = Cast<UInstancedStaticMeshComponent>(SMC)) {
						ActorStats->TriangleCount += Mesh->GetNumTriangles(0) * ISMC->GetInstanceCount();
					}
					else {
						ActorStats->TriangleCount += Mesh->GetNumTriangles(0);
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
			(*CellStats)->DrawCallCount += ActorStats->DrawCallCount;
			(*CellStats)->TriangleCount += ActorStats->TriangleCount;
			(*CellStats)->UsedTextures.Append(CellUsedTextures.Array());
		}
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Cell->GetDebugName());
		return true;
		});
	WorldPartition->FlushStreaming();
	return Stats;
}
