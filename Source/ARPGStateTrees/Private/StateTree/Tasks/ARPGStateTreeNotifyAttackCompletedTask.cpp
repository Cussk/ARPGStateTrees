// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeNotifyAttackCompletedTask.h"

#include "Components/ARPGCombatantComponent.h"
#include "StateTreeExecutionContext.h"

FARPGStateTreeNotifyAttackCompletedTask::FARPGStateTreeNotifyAttackCompletedTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FARPGStateTreeNotifyAttackCompletedTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.CombatantComponent))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CombatantComponent->NotifyAttackCompleted();

	return EStateTreeRunStatus::Succeeded;
}