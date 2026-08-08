// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Conditions/ARPGStateTreeIsNearLocationCondition.h"

#include "StateTreeExecutionContext.h"

bool FARPGStateTreeIsNearLocationCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Actor))
	{
		return false;
	}

	const bool IsNearLocation = FVector::DistSquared2D(InstanceData.Actor->GetActorLocation(), InstanceData.Location) 
	<= FMath::Square(InstanceData.AcceptanceRadius);
	
	return bInverted ? !IsNearLocation : IsNearLocation;
}

FText FARPGStateTreeIsNearLocationCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
	const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return bInverted ? FText::FromString(TEXT("ARPG Not Is Near Location")) : FText::FromString(TEXT("ARPG Is Near Location"));
}
