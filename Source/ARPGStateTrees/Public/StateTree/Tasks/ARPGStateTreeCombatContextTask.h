// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Types/ARPGCombatTypes.h"
#include "ARPGStateTreeCombatContextTask.generated.h"

class AARPGEnemyAIController;
class UARPGCombatantComponent;

USTRUCT()
struct FARPGStateTreeCombatContextTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AARPGEnemyAIController> AIController;

	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, Category = "Output")
	EARPGEngagementState EngagementState = EARPGEngagementState::None;

	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector EngagementLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;

	FDelegateHandle TargetChangedHandle;
	FDelegateHandle CoordinationChangedHandle;
};

/**
 * Routes event-driven combatant state into StateTree-readable outputs.
 */
USTRUCT(meta = (DisplayName = "ARPG Combat Context", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeCombatContextTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeCombatContextTaskInstanceData;

	FARPGStateTreeCombatContextTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	static void UpdateTarget(FInstanceDataType& InstanceData);
	static void UpdateCoordination(FInstanceDataType& InstanceData);
};