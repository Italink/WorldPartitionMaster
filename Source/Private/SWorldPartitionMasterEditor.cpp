#include "SWorldPartitionMasterEditor.h"
#include "Widgets/Layout/SHeader.h"

#define LOCTEXT_NAMESPACE "WorldPartitionMaster"

void SWorldPartitionMasterEditor::Construct(const FArguments& InArgs)
{
	mCellPreviewer.Reset(NewObject<UWorldPartitionStatsCellPreviewer>());

	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	//DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	DetailsViewArgs.bShowObjectLabel = false;
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
								.Text_Lambda([this]() {return FText::FromString(FString::Printf(TEXT("Level: %d"), mCurrentHierarchicalLevel)); })
						]
						+ SHorizontalBox::Slot()[
							SAssignNew(mHierarchicalLevelSlider, SSlider)
								.StepSize(1.0f)
								.Value_Lambda([this]() {return mCurrentHierarchicalLevel; })
								.OnValueChanged(this, &SWorldPartitionMasterEditor::OnHierarchicalLevelChanged)
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
						.Orientation(Orient_Vertical)
						+ SSplitter::Slot()
						[
							SNew(SSpacer)
						]
						+ SSplitter::Slot()
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
									mDetailsView.ToSharedRef()
								]
						]
				]

		];
}

FReply SWorldPartitionMasterEditor::OnClickedFlushStreaming()
{
	UWorld* World = GEditor && GEditor->PlayWorld != nullptr ? GEditor->PlayWorld.Get() : GEditor->GetEditorWorldContext().World();

	mStatsCache = FWorldPartitionStats::FlushStreaming(World);

	mGrids.SetNum(mStatsCache.Grids.Num());
	for (int i = 0; i < mGrids.Num(); i++) {
		mGrids[i] = MakeShared<FWorldPartitionGridStats>(mStatsCache.Grids[i]);
	}
	for (const auto& Grid : mGrids) {
		if (mCurrentGrid && mCurrentGrid->GridName == Grid->GridName) {
			mCurrentGrid = Grid;
			break;
		}
	}
	if (!mCurrentGrid && !mGrids.IsEmpty()) {
		mCurrentGrid = mGrids[0];
	}
	if (mGridComboBox) {
		mGridComboBox->RefreshOptions();
		mGridComboBox->SetSelectedItem(mCurrentGrid);
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SWorldPartitionMasterEditor::OnGenerateGridComboWidget(TSharedPtr<FWorldPartitionGridStats> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(Item->GridName));
}

void SWorldPartitionMasterEditor::OnCurrentGridChanged(TSharedPtr<FWorldPartitionGridStats> Selection, ESelectInfo::Type SelectInfo)
{
	mCurrentGrid = Selection;
	if (mHierarchicalLevelSlider && mCurrentGrid) {
		mHierarchicalLevelSlider->SetMinAndMaxValues(0, mCurrentGrid->MaxHierarchicalLevel);
	}
	OnRefreshCells();
}

void SWorldPartitionMasterEditor::OnHierarchicalLevelChanged(float NewVar)
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
	}
}

#undef LOCTEXT_NAMESPACE