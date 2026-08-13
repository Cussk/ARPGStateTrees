// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeRangedPotshotTask.generated.h"

class AActor;
class UARPGCombatantComponent;

UENUM()
enum class EARPGPotshotMovementMode : uint8
{
	Approach,
	Reposition
};

USTRUCT()
struct FARPGStateTreeRangedPotshotTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;

	UPROPERTY(EditAnywhere, Category = "Input")
	FVector MovementGoal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EARPGPotshotMovementMode MovementMode = EARPGPotshotMovementMode::Reposition;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OpportunityChance = 0.30f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1"))
	float AttackRangeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bWaitForRange = false;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.05"))
	float RangeCheckInterval = 0.20f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MovementGoalTolerance = 50.0f;

	FTimerHandle RangeCheckTimer;

	TWeakObjectPtr<AActor> LastEvaluatedTarget;
	FVector LastEvaluatedMovementGoal = FVector::ZeroVector;

	bool bHasEvaluatedMovement = false;
	bool bPotshotSent = false;
	bool bActive = false;
};

USTRUCT(meta = (DisplayName = "ARPG Ranged Potshot Opportunity", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeRangedPotshotTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeRangedPotshotTaskInstanceData;

	FARPGStateTreeRangedPotshotTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};