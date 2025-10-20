#include "WorldPartitionStatsCustomization.h"
#include "Kismet/GameplayStatics.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Kismet2/CompilerResultsLog.h"
#include "PropertyCustomizationHelpers.h"
#include "IPropertyUtilities.h"
#include "Selection.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SSearchBox.h"
#include "ISinglePropertyView.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "InstancedStructDetails.h"
#include "IStructureDataProvider.h"

#define LOCTEXT_NAMESPACE "WorldPartitionMaster"

class FStructurePropertyDataProvider : public IStructureDataProvider
{
public:
	explicit FStructurePropertyDataProvider(const TSharedRef<IPropertyHandle>& InPropertyHandle) {
		PropertyHandle = InPropertyHandle;
	}

	virtual ~FStructurePropertyDataProvider() override = default;

	//~ Begin IStructureDataProvider
	virtual bool IsValid() const override { return PropertyHandle.IsValid(); }
	virtual const UStruct* GetBaseStructure() const override {
		FStructProperty* StructProperty = (FStructProperty*)(PropertyHandle->GetProperty());
		return StructProperty->Struct;
	}
	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances, const UStruct* ExpectedBaseStructure) const override {
		void* Ptr = nullptr;
		PropertyHandle->GetValueData(Ptr);
		FStructProperty* StructProperty = (FStructProperty*)(PropertyHandle->GetProperty());
		OutInstances.Add(MakeShared<FStructOnScope>(
			StructProperty->Struct,
			reinterpret_cast<uint8*>(Ptr)
		));
	}
	virtual bool IsPropertyIndirection() const override {
		return false;
	}

protected:
	TSharedPtr<IPropertyHandle> PropertyHandle;
};


TSharedRef<IPropertyTypeCustomization> FPropertyTypeCustomization_WorldPartitionActorStats::MakeInstance()
{
	return MakeShared<FPropertyTypeCustomization_WorldPartitionActorStats>();
}

FPropertyTypeCustomization_WorldPartitionActorStats::FPropertyTypeCustomization_WorldPartitionActorStats()
{
}

FPropertyTypeCustomization_WorldPartitionActorStats::~FPropertyTypeCustomization_WorldPartitionActorStats()
{
}

void FPropertyTypeCustomization_WorldPartitionActorStats::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams Params;
	Params.NamePlacement = EPropertyNamePlacement::Hidden;
	StructProvider = MakeShared<FStructurePropertyDataProvider>(InPropertyHandle);
	auto Widget = EditModule.CreateSingleProperty(StructProvider, "Actor", Params);
	void* Ptr = nullptr;
	InPropertyHandle->GetValueData(Ptr);
	Stats = (FWorldPartitionActorStats*)Ptr;
	InHeaderRow
	.NameContent()
	[
		SNew(STextBlock)
			.Text(FText::FromString(Stats->Label))
	]
	.ValueContent()
	[
		Widget.ToSharedRef()
	];
}

void FPropertyTypeCustomization_WorldPartitionActorStats::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex){
		TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex);
		if (ChildHandle->GetPropertyDisplayName().ToString() == "Actor"
			|| ChildHandle->GetPropertyDisplayName().ToString() == "Label")
			continue;
		ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
	}
}
#undef LOCTEXT_NAMESPACE


