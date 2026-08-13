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

	void ScheduleOpportunity(FARPGStateTreeRangedPotshotTaskInstanceData& InstanceData,
		FStateTreeWeakExecutionContext WeakContext,
		TStateTreeInstanceDataStructRef<FARPGStateTreeRangedPotshotTaskInstanceData> InstanceDataRef)
	{
		if (!InstanceData.bActive || InstanceData.bOpportunitySent || !IsValid(InstanceData.CombatantComponent))
		{
			return;
		}

		UWorld* World = InstanceData.CombatantComponent->GetWorld();

		if (!World)
		{
			return;
		}

		const float MaxDelay = FMath::Max(InstanceData.MinDelay, InstanceData.MaxDelay);
		const float Delay = FMath::FRandRange(InstanceData.MinDelay, MaxDelay);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([WeakContext, InstanceDataRef]() mutable
		{
			FARPGStateTreeRangedPotshotTaskInstanceData* Data = InstanceDataRef.GetPtr();

			if (!Data || !Data->bActive || Data->bOpportunitySent || !IsValid(Data->CombatantComponent))
			{
				return;
			}

			if (IsTargetWithinOpportunityRange(*Data))
			{
				Data->bOpportunitySent = true;

				WeakContext.SendEvent(
					ARPGGameplayTags::StateTreeEvent_RangedPotshot,
					FConstStructView(),
					FName(TEXT("RangedPotshot")));

				return;
			}

			ScheduleOpportunity(*Data, WeakContext, InstanceDataRef);
		});

		World->GetTimerManager().SetTimer(InstanceData.OpportunityTimer, TimerDelegate, Delay, false);
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
	InstanceData.bOpportunitySent = false;

	ARPGStateTreeRangedPotshot::ScheduleOpportunity(
		InstanceData,
		Context.MakeWeakExecutionContext(),
		Context.GetInstanceDataStructRef(*this));

	return EStateTreeRunStatus::Running;
}

void FARPGStateTreeRangedPotshotTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.bActive = false;
	InstanceData.bOpportunitySent = false;

	if (IsValid(InstanceData.CombatantComponent))
	{
		if (UWorld* World = InstanceData.CombatantComponent->GetWorld())
		{
			World->GetTimerManager().ClearTimer(InstanceData.OpportunityTimer);
		}
	}

	InstanceData.OpportunityTimer.Invalidate();
}