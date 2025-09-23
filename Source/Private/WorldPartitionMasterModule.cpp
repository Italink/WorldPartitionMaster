#include "WorldPartitionMasterModule.h"
#include "EditorModeRegistry.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "SWorldPartitionMasterEditor.h"

#define LOCTEXT_NAMESPACE "WorldPartitionMaster"

void FWorldPartitionMasterModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TEXT("WorldPartitionMaster"),
		FOnSpawnTab::CreateStatic(&FWorldPartitionMasterModule::SpawnWorldPartitionMaster))
		.SetDisplayName(NSLOCTEXT("UnrealEditor", "WorldPartitionMasterTab", "World Partition Master"))
		.SetTooltipText(NSLOCTEXT("UnrealEditor", "WorldPartitionMasterTooltipText", "Open the World Partition Master Tab."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorWorldPartitionCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Debug"))
		.SetCanSidebarTab(false);
}

void FWorldPartitionMasterModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized()){
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("WorldPartitionMaster"));
	}
}


TSharedRef<SDockTab> FWorldPartitionMasterModule::SpawnWorldPartitionMaster(const FSpawnTabArgs& Args)
{
	auto NomadTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WorldPartitionMasterTab", "World Partition Master"));

	auto TabManager = FGlobalTabmanager::Get()->NewTabManager(NomadTab);
	TabManager->SetOnPersistLayout(
		FTabManager::FOnPersistLayout::CreateStatic(
			[](const TSharedRef<FTabManager::FLayout>& InLayout)
			{
				if (InLayout->GetPrimaryArea().Pin().IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, InLayout);
				}
			}
		)
	);
	NomadTab->SetOnTabClosed(
		SDockTab::FOnTabClosedCallback::CreateStatic(
			[](TSharedRef<SDockTab> Self, TWeakPtr<FTabManager> TabManager)
			{
				TSharedPtr<FTabManager> OwningTabManager = TabManager.Pin();
				if (OwningTabManager.IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, OwningTabManager->PersistLayout());
					OwningTabManager->CloseAllAreas();
				}
			}
			, TWeakPtr<FTabManager>(TabManager)
		)
	);

	auto MainWidget = SNew(SWorldPartitionMasterEditor)
		.TabManager(TabManager);

	NomadTab->SetContent(MainWidget);
	return NomadTab;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWorldPartitionMasterModule, WorldPartitionMaster)