#include "SWorldPartitionMasterEditor.h"
#include "Widgets/Layout/SHeader.h"
#include "LevelEditor.h"
#include "WorldPartitionMasterEditorSettings.h"
#include "Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PostProcessComponent.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/HLOD/HLODActor.h"

#define LOCTEXT_NAMESPACE "WorldPartitionMaster"

UWorld* SWorldPartitionMasterEditor::GetWorld()
{
	return GEditor && GEditor->PlayWorld != nullptr ? GEditor->PlayWorld.Get() : GEditor->GetEditorWorldContext().World();
}

void SWorldPartitionMasterEditor::Construct(const FArguments& InArgs)
{
	mCellPreviewer.Reset(NewObject<UWorldPartitionStatsCellPreviewer>());

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	OnActorSelectionChangedHandle = LevelEditor.OnActorSelectionChanged().AddSP(this, &SWorldPartitionMasterEditor::OnActorSelectionChanged);

	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	//DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.bShowActorLabel = false;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bAllowFavoriteSystem = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.ViewIdentifier = FName("BlueprintDefaults");
	mDetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	mDetailsView->SetObject(mCellPreviewer.Get());

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				.Padding(5)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(FText::FromString(TEXT("Grid:")))
						]
						+ SHorizontalBox::Slot()[
							SAssignNew(mGridComboBox, SComboBox<TSharedPtr<FWorldPartitionGridStats>>)
								.OptionsSource(&mGrids)
								.OnSelectionChanged(this, &SWorldPartitionMasterEditor::OnCurrentGridChanged)
								.OnGenerateWidget(this, &SWorldPartitionMasterEditor::OnGenerateGridComboWidget)
								[
									SNew(STextBlock)
										.Text_Lambda([this]() {
											return mCurrentGrid ? FText::FromName(mCurrentGrid->GridName) : FText::FromString("Null");
										})
								]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text_Lambda([this]() {return FText::FromString(TEXT("Level:")); })
						]
						+ SHorizontalBox::Slot()[
							SAssignNew(mHierarchicalLevelBox, SSpinBox<int32>)
								.Value_Lambda([this]() {return mCurrentHierarchicalLevel; })
								.OnValueChanged(this, &SWorldPartitionMasterEditor::OnHierarchicalLevelChanged)
								.MinValue(0)
								.MinSliderValue(0)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10, 2)
						[
							SNew(SButton)
								.Text(LOCTEXT("Flush", "FlushStreaming"))
								.OnClicked(this, &SWorldPartitionMasterEditor::OnClickedFlushStreaming)
						]
				]
				+ SVerticalBox::Slot()
				.FillHeight(5)
				[
					SNew(SSplitter)
						+ SSplitter::Slot()
						.Value(0.25)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHeader)
										[
											SNew(STextBlock)
												.Text_Lambda([this]() {
													return FText::FromString(FString::Printf(TEXT(" Cells [%d] "), mCells.Num()));
												})
										]

								]
								+ SVerticalBox::Slot()
								[
									SAssignNew(mCellListView, SListView<TSharedPtr<FWorldPartitionCellStats>>)
										.ListItemsSource(&mCells)
										.ScrollbarVisibility(EVisibility::Visible)
										.OnGenerateRow(this, &SWorldPartitionMasterEditor::OnGenerateCellRow)
										.OnSelectionChanged(this, &SWorldPartitionMasterEditor::OnCellSelectionChanged)
								]
						]
						+ SSplitter::Slot()
						.Value(0.75)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
												.Text(LOCTEXT("ToggleIsolate", "Toggle Isolate"))
												.OnClicked(this, &SWorldPartitionMasterEditor::OnClickedToggleIsolate)
										]
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
												.Text(LOCTEXT("ToggleHLOD", "Toggle HLOD"))
												.OnClicked(this, &SWorldPartitionMasterEditor::OnClickedToggleHLOD)
										]

								]
								+ SVerticalBox::Slot()

								[
									mDetailsView.ToSharedRef()
								]
							
						]
				]
		];


		mStatsCache = UWorldPartitionMasterEditorSettings::Get()->FindOrGetStats(GetWorld()->GetFName());
		RebuildEditorStreaming();
}

FReply SWorldPartitionMasterEditor::OnClickedFlushStreaming()
{
	UWorld* World= GetWorld();
	mCellPreviewer->World = World;
	mStatsCache = FWorldPartitionStats::FlushStreaming(World);
	UWorldPartitionMasterEditorSettings::Get()->SetStats(World->GetFName(), mStatsCache);
	RebuildEditorStreaming();
	return FReply::Handled();
}

void SWorldPartitionMasterEditor::RebuildEditorStreaming()
{
	mActor2CellLUT.Reset();
	mGrids.SetNum(mStatsCache.Grids.Num());

	for (int i = 0; i < mGrids.Num(); i++) {
		mGrids[i] = MakeShared<FWorldPartitionGridStats>(mStatsCache.Grids[i]);
	}
	for (const auto& Grid : mGrids) {
		if (mCurrentGrid && mCurrentGrid->GridName == Grid->GridName) {
			mCurrentGrid = Grid;
			break;
		}
		for (const auto& Cell : Grid->Cells) {
			for (const auto& Actor : Cell.Actors) {
				mActor2CellLUT.Add(Actor.ActorGuid, { Grid->GridName, Cell.CellName });
			}
		}
	}
	if (!mCurrentGrid && !mGrids.IsEmpty()) {
		mCurrentGrid = mGrids[0];
	}
	if (mGridComboBox) {
		mGridComboBox->RefreshOptions();
		mGridComboBox->SetSelectedItem(mCurrentGrid);
	}
}

TSharedRef<SWidget> SWorldPartitionMasterEditor::OnGenerateGridComboWidget(TSharedPtr<FWorldPartitionGridStats> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(Item->GridName));
}

void SWorldPartitionMasterEditor::OnCurrentGridChanged(TSharedPtr<FWorldPartitionGridStats> Selection, ESelectInfo::Type SelectInfo)
{
	mCurrentGrid = Selection;
	if (mHierarchicalLevelBox && mCurrentGrid) {
		mCurrentHierarchicalLevel = FMath::Clamp(mCurrentHierarchicalLevel, 0, mCurrentGrid->MaxHierarchicalLevel);
		mHierarchicalLevelBox->SetMaxValue(mCurrentGrid->MaxHierarchicalLevel);
		mHierarchicalLevelBox->SetMaxSliderValue(mCurrentGrid->MaxHierarchicalLevel);
	}
	OnRefreshCells();
}

void SWorldPartitionMasterEditor::OnHierarchicalLevelChanged(int32 NewVar)
{
	mCurrentHierarchicalLevel = NewVar;
	OnRefreshCells();
}

void SWorldPartitionMasterEditor::OnRefreshCells()
{
	mCells.Reset();
	if (mCurrentGrid) {
		for (auto Cell : mCurrentGrid->Cells) {
			if (Cell.HierarchicalLevel == mCurrentHierarchicalLevel) {
				mCells.Add(MakeShared<FWorldPartitionCellStats>(Cell));
			}
		}
		if (mCellListView) {
			mCellListView->RebuildList();
		}
	}
}

TSharedRef<ITableRow> SWorldPartitionMasterEditor::OnGenerateCellRow(TSharedPtr<FWorldPartitionCellStats> Cell, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedRef<FWorldPartitionCellStats>>, OwnerTable)
		[
			SNew(STextBlock)
				.Text(FText::FromName(Cell->CellName))
		];
}

void SWorldPartitionMasterEditor::OnCellSelectionChanged(TSharedPtr<FWorldPartitionCellStats> Cell, ESelectInfo::Type Info)
{
	mCurrentCell = Cell;

	if (mCurrentCell) {
		mCellPreviewer->CellStats = *Cell;
		for (auto& ActorStats : mCurrentCell->Actors) {
			ActorStats.Actor = ActorStats.Path.ResolveObject();
		}

		if (!bCellChangedByActorSelection) {
			GEditor->GetSelectedActors()->Modify();
			GEditor->GetSelectedActors()->BeginBatchSelectOperation();
			GEditor->SelectNone(false, true, true);

			FBox Bounds = mCurrentCell->Bounds;
			Bounds.Min.Z = FLT_MAX;
			Bounds.Max.Z = FLT_MIN;

			for (auto ActorStats : mCurrentCell->Actors) {
				if (ActorStats.Actor) {
					AActor* Actor = ActorStats.Actor.Get();
					Actor->SetIsTemporarilyHiddenInEditor(false);
					GEditor->SelectActor(Actor, true, false, true);

					FVector ActorOrigin;   
					FVector ActorExtent;    
					Actor->GetActorBounds(false, ActorOrigin, ActorExtent);  

					float ActorMinZ = ActorOrigin.Z - ActorExtent.Z;  
					float ActorMaxZ = ActorOrigin.Z + ActorExtent.Z;  

					Bounds.Min.Z = FMath::Min(Bounds.Min.Z, ActorMinZ);
					Bounds.Max.Z = FMath::Max(Bounds.Max.Z, ActorMaxZ);
				}
			}

			if (Bounds.Min.Z == FLT_MAX || Bounds.Max.Z == FLT_MIN) {
				Bounds.Min.Z = 0.0f;
				Bounds.Max.Z = 10000.0f;
			}

			GEditor->MoveViewportCamerasToBox(Bounds, true);
			UWorld* World = GetWorld();
			DrawDebugBox(World, Bounds.GetCenter(), Bounds.GetExtent(), FColor(255, 0, 0), false, 2, 0, 400);

			GEditor->GetSelectedActors()->EndBatchSelectOperation(/*bNotify*/false);
			GEditor->NoteSelectionChange();
		}
	}
	if (!HiddenActors.IsEmpty()) {
		HiddenActors.Empty();
		OnClickedToggleIsolate();
	}
}

void SWorldPartitionMasterEditor::OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	if (NewSelection.Num() != 1) {
		return;
	}
	AActor* Actor = Cast<AActor>(NewSelection[0]);
	if (Cast<AWorldPartitionHLOD>(Actor)) {
		return;
	}

	if (auto GridCellPair = mActor2CellLUT.Find(Actor->GetActorGuid())) {
		bCellChangedByActorSelection = true;

		if (!mCurrentGrid || (mCurrentGrid && mCurrentGrid->GridName != GridCellPair->Key)) {
			for (auto Grid : mGrids) {
				if (Grid->GridName == GridCellPair->Key) {
					mGridComboBox->SetSelectedItem(Grid);
					break;
				}
			}
		}

		if (!mCurrentCell || (mCurrentCell && mCurrentCell->CellName != GridCellPair->Value)) {
			for (auto Cell : mCells) {
				if (Cell->CellName == GridCellPair->Value) {
					mCellListView->SetSelection(Cell);
					break;
				}
			}
		}

		bCellChangedByActorSelection = false;
	}
}

FReply SWorldPartitionMasterEditor::OnClickedToggleIsolate()
{
	if (!mCurrentCell) {
		return FReply::Handled();
	}
	if (!HiddenActors.IsEmpty()) {
		for (auto Actor : HiddenActors) {
			if (Actor) {
				Actor->SetIsTemporarilyHiddenInEditor(false);
			}
		}
		HiddenActors.Empty();
		return FReply::Handled();
	}
	UWorld* World = GetWorld();
	UWorldPartition* WorldPartition = World->GetWorldPartition();
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
	for (auto Actor : AllActors) {
		if (Actor->GetActorLabel().Contains("Sky"))
			continue;
		if (Actor->GetComponentByClass(ULightComponent::StaticClass())
			|| Actor->GetComponentByClass(USkyAtmosphereComponent::StaticClass())
			|| Actor->GetComponentByClass(UVolumetricCloudComponent::StaticClass())
			|| Actor->GetComponentByClass(UPostProcessComponent::StaticClass())
			)
			continue;

		FWorldPartitionActorStats* ActorStats = mCurrentCell->Actors.FindByPredicate([Actor](const FWorldPartitionActorStats& ActorStats) {
			return ActorStats.ActorGuid == Actor->GetActorGuid();
			});
		if (ActorStats != nullptr || mCurrentCell->Hlod.HlodActor.ActorGuid == Actor->GetActorGuid())
			continue;
		Actor->SetIsTemporarilyHiddenInEditor(true);
		HiddenActors.Add(Actor);
	}
	for (auto& Actor : mCurrentCell->Actors) {
		Actor.Actor = Actor.Path.ResolveObject();
		if (!Actor.Actor) {
			WorldPartition->PinActors({ Actor.ActorGuid });
			Actor.Actor = Actor.Path.ResolveObject();
		}
	}
	return FReply::Handled();
}

FReply SWorldPartitionMasterEditor::OnClickedToggleHLOD()
{
	if (!mCurrentCell) {
		return FReply::Handled();
	}
	UWorld* World = GetWorld();
	UWorldPartition* WorldPartition = World->GetWorldPartition();
	mCurrentCell->Hlod.HlodActor.Actor = mCurrentCell->Hlod.HlodActor.Path.ResolveObject();
	if (!mCurrentCell->Hlod.HlodActor.Actor || (mCurrentCell->Hlod.HlodActor.Actor && mCurrentCell->Hlod.HlodActor.Actor->IsTemporarilyHiddenInEditor())) {
		if (!mCurrentCell->Hlod.HlodActor.Actor) {
			WorldPartition->PinActors({ mCurrentCell->Hlod.HlodActor.ActorGuid });
			mCurrentCell->Hlod.HlodActor.Actor = mCurrentCell->Hlod.HlodActor.Path.ResolveObject();
		}
		if (mCurrentCell->Hlod.HlodActor.Actor) {
			mCurrentCell->Hlod.HlodActor.Actor->SetIsTemporarilyHiddenInEditor(false);
		}
		for (auto& Actor : mCurrentCell->Actors) {
			Actor.Actor = Actor.Path.ResolveObject();
			if (Actor.Actor) {
				Actor.Actor->SetIsTemporarilyHiddenInEditor(true);
			}
		}
	}
	else {
		mCurrentCell->Hlod.HlodActor.Actor->SetIsTemporarilyHiddenInEditor(true);
		for (auto& Actor : mCurrentCell->Actors) {
			Actor.Actor = Actor.Path.ResolveObject();
			if (!Actor.Actor) {
				WorldPartition->PinActors({ Actor.ActorGuid });
				Actor.Actor = Actor.Path.ResolveObject();
			}
			if (Actor.Actor) {
				Actor.Actor->SetIsTemporarilyHiddenInEditor(false);
			}
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE