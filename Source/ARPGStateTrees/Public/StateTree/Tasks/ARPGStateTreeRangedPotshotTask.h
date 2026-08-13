// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeRangedPotshotTask.generated.h"

class UARPGCombatantComponent;

USTRUCT()
struct FARPGStateTreeRangedPotshotTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MaxDelay = 0.40f;
	
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1"))
	float AttackRangeMultiplier = 1.0f;

	FTimerHandle OpportunityTimer;

	bool bActive = false;
	bool bOpportunitySent = false;
};

USTRUCT(meta = (DisplayName = "ARPG Ranged Potshot Timer", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeRangedPotshotTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeRangedPotshotTaskInstanceData;

	FARPGStateTreeRangedPotshotTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
