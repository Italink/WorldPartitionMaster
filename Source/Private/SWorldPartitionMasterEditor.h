#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "WorldPartitionStats.h"

class SWorldPartitionMasterEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldPartitionMasterEditor) {}
		SLATE_ARGUMENT(TSharedPtr<class FTabManager>, TabManager)
	SLATE_END_ARGS()
public:
	UWorld* GetWorld();

	void Construct(const FArguments& InArgs);

	FReply OnClickedFlushStreaming();
	void RebuildEditorStreaming();

	TSharedRef<SWidget> OnGenerateGridComboWidget(TSharedPtr<FWorldPartitionGridStats> Item) const;
	void OnCurrentGridChanged(TSharedPtr<FWorldPartitionGridStats> Selection, ESelectInfo::Type SelectInfo);

	void OnHierarchicalLevelChanged(int32 NewVar);

	void OnRefreshCells();

	TSharedRef<ITableRow> OnGenerateCellRow(TSharedPtr<FWorldPartitionCellStats> Cell, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCellSelectionChanged(TSharedPtr<FWorldPartitionCellStats> Cell, ESelectInfo::Type Info);

	FDelegateHandle OnActorSelectionChangedHandle;
	void OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh);

	FReply OnClickedToggleIsolate();
	FReply OnClickedToggleHLOD();
protected:
	TSharedPtr<FTabManager> TabManager;

	FWorldPartitionStats mStatsCache;

	TSharedPtr<FWorldPartitionGridStats> mCurrentGrid;
	TArray<TSharedPtr<FWorldPartitionGridStats>> mGrids;
	TSharedPtr<SComboBox<TSharedPtr<FWorldPartitionGridStats>>> mGridComboBox;

	int mCurrentHierarchicalLevel = 0;
	TSharedPtr<SSpinBox<int32>> mHierarchicalLevelBox;

	TSharedPtr<FWorldPartitionCellStats> mCurrentCell;
	TArray<TSharedPtr<FWorldPartitionCellStats>> mCells;
	TSharedPtr<SListView<TSharedPtr<FWorldPartitionCellStats>>> mCellListView;

	TSharedPtr<IDetailsView> mDetailsView;

	TMap<FGuid, TPair<FName, FName>> mActor2CellLUT;

	TStrongObjectPtr<UWorldPartitionStatsCellPreviewer> mCellPreviewer;

	bool bCellChangedByActorSelection = false;

	TArray<TObjectPtr<AActor>> HiddenActors;
};