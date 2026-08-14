// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeNotifySupportAbilityUsedTask.h"

#include "Components/ARPGCombatantComponent.h"
#include "StateTreeExecutionContext.h"

FARPGStateTreeNotifySupportAbilityUsedTask::FARPGStateTreeNotifySupportAbilityUsedTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FARPGStateTreeNotifySupportAbilityUsedTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.CombatantComponent))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CombatantComponent->NotifySupportAbilityUsed();

	return EStateTreeRunStatus::Succeeded;
}
