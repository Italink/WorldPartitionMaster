#pragma once

#include "WorldPartitionStats.h"
#include "IPropertyTypeCustomization.h"

struct FProceduralObjectMatrixRow;

class FPropertyTypeCustomization_WorldPartitionActorStats
	: public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	FPropertyTypeCustomization_WorldPartitionActorStats();
	~FPropertyTypeCustomization_WorldPartitionActorStats();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)override;

private:
	TSharedPtr<IPropertyUtilities> Utils;
	TSharedPtr<IPropertyHandle> Handle;
	TSharedPtr<IStructureDataProvider> StructProvider;
	FWorldPartitionActorStats* Stats = nullptr;
};