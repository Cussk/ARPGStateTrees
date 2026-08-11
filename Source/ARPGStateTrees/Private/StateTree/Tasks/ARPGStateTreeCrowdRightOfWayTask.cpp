// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeCrowdRightOfWayTask.h"

#include "Character/ARPGEnemyCharacter.h"
#include "Components/ARPGCrowdMovementComponent.h"
#include "StateTreeExecutionContext.h"

FARPGStateTreeCrowdRightOfWayTask::FARPGStateTreeCrowdRightOfWayTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bConsideredForCompletion = false;
}

EStateTreeRunStatus FARPGStateTreeCrowdRightOfWayTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Character))
	{
		return EStateTreeRunStatus::Failed;
	}

	UARPGCrowdMovementComponent* CrowdMovement = InstanceData.Character->GetCrowdMovementComponent();

	if (!IsValid(CrowdMovement))
	{
		return EStateTreeRunStatus::Failed;
	}

	CrowdMovement->SetRightOfWayRequested(true);

	return EStateTreeRunStatus::Running;
}

void FARPGStateTreeCrowdRightOfWayTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Character))
	{
		return;
	}

	if (UARPGCrowdMovementComponent* CrowdMovement = InstanceData.Character->GetCrowdMovementComponent())
	{
		CrowdMovement->SetRightOfWayRequested(false);
	}
}