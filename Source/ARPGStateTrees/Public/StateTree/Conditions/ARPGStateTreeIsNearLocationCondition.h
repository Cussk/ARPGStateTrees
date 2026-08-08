// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "ARPGStateTreeIsNearLocationCondition.generated.h"

USTRUCT()
struct FARPGStateTreeIsNearLocationConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Actor;

	UPROPERTY(EditAnywhere, Category = "Input")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 75.0f;
};

/**
 * Tests whether an actor is within a 2D acceptance radius of a location.
 */
USTRUCT(DisplayName = "ARPG Is Near Location")
struct ARPGSTATETREES_API FARPGStateTreeIsNearLocationCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInverted = false;

	using FInstanceDataType = FARPGStateTreeIsNearLocationConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const;
};