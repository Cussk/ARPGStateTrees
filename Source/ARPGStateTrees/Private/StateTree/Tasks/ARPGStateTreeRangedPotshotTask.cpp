// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeRangedPotshotTask.h"

#include "Components/ARPGCombatantComponent.h"
#include "Types/ARPGGameplayTags.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"

namespace ARPGStateTreeRangedPotshot
{
	bool IsTargetWithinOpportunityRange(const FARPGStateTreeRangedPotshotTaskInstanceData& InstanceData)
	{
		if (!IsValid(InstanceData.CombatantComponent))
		{
			return false;
		}

		const UARPGCombatantComponent* TargetCombatant = InstanceData.CombatantComponent->GetCurrentTarget();

		if (!IsValid(TargetCombatant))
		{
			return false;
		}

		const AActor* AttackerActor = InstanceData.CombatantComponent->GetCombatantActor();
		const AActor* TargetActor = TargetCombatant->GetCombatantActor();

		if (!IsValid(AttackerActor) || !IsValid(TargetActor))
		{
			return false;
		}

		const float OpportunityRange = InstanceData.CombatantComponent->GetBasicAttackRange()
			* InstanceData.AttackRangeMultiplier;

		return FVector::DistSquared2D(AttackerActor->GetActorLocation(), TargetActor->GetActorLocation())
			<= FMath::Square(OpportunityRange);
	}

	bool IsNewMovement(const FARPGStateTreeRangedPotshotTaskInstanceData& InstanceData)
	{
		if (!InstanceData.bHasEvaluatedMovement)
		{
			return true;
		}

		if (InstanceData.MovementMode == EARPGPotshotMovementMode::Approach)
		{
			if (!IsValid(InstanceData.CombatantComponent))
			{
				return false;
			}

			const UARPGCombatantComponent* CurrentTarget = InstanceData.CombatantComponent->GetCurrentTarget();

			return InstanceData.LastEvaluatedTarget.Get() != CurrentTarget->GetCombatantActor();
		}

		return FVector::DistSquared2D(InstanceData.LastEvaluatedMovementGoal, InstanceData.MovementGoal)
			> FMath::Square(InstanceData.MovementGoalTolerance);
	}
	
	void RecordMovement(FARPGStateTreeRangedPotshotTaskInstanceData& InstanceData)
	{
		InstanceData.bHasEvaluatedMovement = true;
		InstanceData.bPotshotSent = false;

		if (InstanceData.MovementMode == EARPGPotshotMovementMode::Approach)
		{
			const UARPGCombatantComponent* CurrentTarget = InstanceData.CombatantComponent->GetCurrentTarget();
			InstanceData.LastEvaluatedTarget = CurrentTarget->GetCombatantActor();
			return;
		}

		InstanceData.LastEvaluatedMovementGoal = InstanceData.MovementGoal;
	}

	void SendPotshot(FARPGStateTreeRangedPotshotTaskInstanceData& InstanceData, FStateTreeWeakExecutionContext WeakContext)
	{
		if (InstanceData.bPotshotSent)
		{
			return;
		}

		InstanceData.bPotshotSent = true;

		if (IsValid(InstanceData.CombatantComponent))
		{
			if (UWorld* World = InstanceData.CombatantComponent->GetWorld())
			{
				World->GetTimerManager().ClearTimer(InstanceData.RangeCheckTimer);
			}
		}

		WeakContext.SendEvent(
			ARPGGameplayTags::StateTreeEvent_RangedPotshot,
			FConstStructView(),
			FName(TEXT("RangedPotshot")));
	}

	void StartRangeCheck(FARPGStateTreeRangedPotshotTaskInstanceData& InstanceData,
		FStateTreeWeakExecutionContext WeakContext,
		TStateTreeInstanceDataStructRef<FARPGStateTreeRangedPotshotTaskInstanceData> InstanceDataRef)
	{
		if (!InstanceData.bActive || InstanceData.bPotshotSent || !IsValid(InstanceData.CombatantComponent))
		{
			return;
		}

		UWorld* World = InstanceData.CombatantComponent->GetWorld();

		if (!World)
		{
			return;
		}

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([WeakContext, InstanceDataRef]() mutable
		{
			FARPGStateTreeRangedPotshotTaskInstanceData* Data = InstanceDataRef.GetPtr();

			if (!Data || !Data->bActive || Data->bPotshotSent || !IsValid(Data->CombatantComponent))
			{
				return;
			}

			if (!IsTargetWithinOpportunityRange(*Data))
			{
				return;
			}

			SendPotshot(*Data, WeakContext);
		});

		World->GetTimerManager().SetTimer(
			InstanceData.RangeCheckTimer,
			TimerDelegate,
			InstanceData.RangeCheckInterval,
			true);
	}
}

FARPGStateTreeRangedPotshotTask::FARPGStateTreeRangedPotshotTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bConsideredForCompletion = false;
}

EStateTreeRunStatus FARPGStateTreeRangedPotshotTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.CombatantComponent))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bActive = true;

	if (!ARPGStateTreeRangedPotshot::IsNewMovement(InstanceData))
	{
		return EStateTreeRunStatus::Running;
	}

	ARPGStateTreeRangedPotshot::RecordMovement(InstanceData);

	if (FMath::FRand() > InstanceData.OpportunityChance)
	{
		return EStateTreeRunStatus::Running;
	}

	const FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();

	if (ARPGStateTreeRangedPotshot::IsTargetWithinOpportunityRange(InstanceData))
	{
		ARPGStateTreeRangedPotshot::SendPotshot(InstanceData, WeakContext);
		return EStateTreeRunStatus::Running;
	}

	if (InstanceData.bWaitForRange)
	{
		ARPGStateTreeRangedPotshot::StartRangeCheck(
			InstanceData,
			WeakContext,
			Context.GetInstanceDataStructRef(*this));
	}

	return EStateTreeRunStatus::Running;
}

void FARPGStateTreeRangedPotshotTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.bActive = false;

	if (IsValid(InstanceData.CombatantComponent))
	{
		if (UWorld* World = InstanceData.CombatantComponent->GetWorld())
		{
			World->GetTimerManager().ClearTimer(InstanceData.RangeCheckTimer);
		}
	}

	InstanceData.RangeCheckTimer.Invalidate();
}