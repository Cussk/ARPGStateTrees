// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeRandomChanceTask.h"

#include "StateTreeExecutionContext.h"

FARPGStateTreeRandomChanceTask::FARPGStateTreeRandomChanceTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FARPGStateTreeRandomChanceTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	return FMath::FRand() <= InstanceData.Chance
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Failed;
}