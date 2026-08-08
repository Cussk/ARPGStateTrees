// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeWaitForCombatContextTask.h"

#include "StateTreeExecutionContext.h"

FARPGStateTreeWaitForCombatContextTask::FARPGStateTreeWaitForCombatContextTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FARPGStateTreeWaitForCombatContextTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return EStateTreeRunStatus::Running;
}