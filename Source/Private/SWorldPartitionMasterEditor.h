#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSlider.h"
#include "WorldPartitionStats.h"

class SWorldPartitionMasterEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldPartitionMasterEditor) {}
		SLATE_ARGUMENT(TSharedPtr<class FTabManager>, TabManager)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);
	FReply OnClickedFlushStreaming();

	TSharedRef<SWidget> OnGenerateGridComboWidget(TSharedPtr<FWorldPartitionGridStats> Item) const;
	void OnCurrentGridChanged(TSharedPtr<FWorldPartitionGridStats> Selection, ESelectInfo::Type SelectInfo);

	void OnHierarchicalLevelChanged(float NewVar);

	void OnRefreshCells();

	TSharedRef<ITableRow> OnGenerateCellRow(TSharedPtr<FWorldPartitionCellStats> Cell, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCellSelectionChanged(TSharedPtr<FWorldPartitionCellStats> Cell, ESelectInfo::Type Info);

	TSharedRef<ITableRow> OnGenerateActorRow(TSharedPtr<FWorldPartitionActorStats> Actor, const TSharedRef<STableViewBase>& OwnerTable);
	void OnActorSelectionChanged(TSharedPtr<FWorldPartitionActorStats> Actor, ESelectInfo::Type Info);
protected:
	TSharedPtr<FTabManager> TabManager;

	TSharedPtr<FWorldPartitionGridStats> mCurrentGrid;
	TArray<TSharedPtr<FWorldPartitionGridStats>> mGrids;
	TSharedPtr<SComboBox<TSharedPtr<FWorldPartitionGridStats>>> mGridComboBox;

	int mCurrentHierarchicalLevel = 0;
	TSharedPtr<SSlider> mHierarchicalLevelSlider;

	TSharedPtr<FWorldPartitionCellStats> mCurrentCell;
	TArray<TSharedPtr<FWorldPartitionCellStats>> mCells;
	TSharedPtr<SListView<TSharedPtr<FWorldPartitionCellStats>>> mCellListView;

	TSharedPtr<FWorldPartitionActorStats> mCurrentActor;
	TArray<TSharedPtr<FWorldPartitionActorStats>> mActors;
	TSharedPtr<SListView<TSharedPtr<FWorldPartitionActorStats>>> mActorsListView;

	TSharedPtr<FWorldPartitionStats> mStats;
};