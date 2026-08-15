// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeCompanionContextTask.h"

#include "StateTreeAsyncExecutionContext.h"
#include "Components/ARPGCompanionComponent.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"
#include "Types/ARPGGameplayTags.h"

namespace ARPGStateTreeCompanionContext
{
	void EvaluateFollowState(FARPGStateTreeCompanionContextTaskInstanceData& InstanceData,
		const FStateTreeWeakExecutionContext& WeakContext)
	{
		if (!InstanceData.bActive || !IsValid(InstanceData.CompanionComponent)
			|| !IsValid(InstanceData.CompanionActor))
		{
			return;
		}

		AActor* OwnerActor = InstanceData.CompanionComponent->GetCompanionOwnerActor();
		InstanceData.CompanionOwnerActor = OwnerActor;

		if (!IsValid(OwnerActor))
		{
			InstanceData.bShouldFollow = false;
			return;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			InstanceData.CompanionActor->GetActorLocation(),
			OwnerActor->GetActorLocation());

		bool bNewShouldFollow = InstanceData.bShouldFollow;

		if (InstanceData.bShouldFollow)
		{
			if (DistanceSquared <= FMath::Square(InstanceData.CompanionComponent->GetFollowDistance()))
			{
				bNewShouldFollow = false;
			}
		}
		else
		{
			if (DistanceSquared >= FMath::Square(InstanceData.CompanionComponent->GetCatchUpDistance()))
			{
				bNewShouldFollow = true;
			}
		}

		if (bNewShouldFollow == InstanceData.bShouldFollow)
		{
			return;
		}

		InstanceData.bShouldFollow = bNewShouldFollow;

		WeakContext.SendEvent(
			bNewShouldFollow
				? ARPGGameplayTags::StateTreeEvent_CompanionFollowRequired
				: ARPGGameplayTags::StateTreeEvent_CompanionFollowSatisfied,
			FConstStructView(),
			bNewShouldFollow
				? FName(TEXT("CompanionFollowRequired"))
				: FName(TEXT("CompanionFollowSatisfied")));
	}

	void StartFollowCheck(FARPGStateTreeCompanionContextTaskInstanceData& InstanceData,
		FStateTreeWeakExecutionContext WeakContext,
		TStateTreeInstanceDataStructRef<FARPGStateTreeCompanionContextTaskInstanceData> InstanceDataRef)
	{
		if (!IsValid(InstanceData.CompanionActor))
		{
			return;
		}

		UWorld* World = InstanceData.CompanionActor->GetWorld();

		if (!World)
		{
			return;
		}

		FTimerDelegate TimerDelegate;

		TimerDelegate.BindLambda([WeakContext, InstanceDataRef]() mutable
		{
			FARPGStateTreeCompanionContextTaskInstanceData* Data = InstanceDataRef.GetPtr();

			if (!Data)
			{
				return;
			}

			EvaluateFollowState(*Data, WeakContext);
		});

		const float Interval = IsValid(InstanceData.CompanionComponent)
			? InstanceData.CompanionComponent->GetFollowUpdateInterval()
			: 0.25f;

		World->GetTimerManager().SetTimer(
			InstanceData.FollowCheckTimer,
			TimerDelegate,
			Interval,
			true);
	}
}

FARPGStateTreeCompanionContextTask::FARPGStateTreeCompanionContextTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bConsideredForCompletion = false;
}

EStateTreeRunStatus FARPGStateTreeCompanionContextTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.CompanionActor))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CompanionComponent =
		InstanceData.CompanionActor->FindComponentByClass<UARPGCompanionComponent>();

	if (!IsValid(InstanceData.CompanionComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("ARPG Companion Context: %s has no ARPGCompanionComponent."),
			*GetNameSafe(InstanceData.CompanionActor));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bActive = true;
	InstanceData.CompanionOwnerActor = InstanceData.CompanionComponent->GetCompanionOwnerActor();

	const FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();
	const TStateTreeInstanceDataStructRef<FInstanceDataType> InstanceDataRef = Context.GetInstanceDataStructRef(*this);

	InstanceData.OwnerChangedHandle = InstanceData.CompanionComponent->OnCompanionOwnerChanged.AddLambda(
		[WeakContext, InstanceDataRef](AActor* NewOwner) mutable
		{
			FInstanceDataType* Data = InstanceDataRef.GetPtr();

			if (!Data || !Data->bActive)
			{
				return;
			}

			Data->CompanionOwnerActor = NewOwner;
			Data->bShouldFollow = false;

			WeakContext.SendEvent(
				ARPGGameplayTags::StateTreeEvent_CompanionOwnerChanged,
				FConstStructView(),
				FName(TEXT("CompanionOwnerChanged")));

			ARPGStateTreeCompanionContext::EvaluateFollowState(*Data, WeakContext);
		});

	ARPGStateTreeCompanionContext::EvaluateFollowState(InstanceData, WeakContext);

	ARPGStateTreeCompanionContext::StartFollowCheck(
		InstanceData,
		WeakContext,
		InstanceDataRef);

	return EStateTreeRunStatus::Running;
}

void FARPGStateTreeCompanionContextTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.bActive = false;

	if (IsValid(InstanceData.CompanionComponent))
	{
		InstanceData.CompanionComponent->OnCompanionOwnerChanged.Remove(InstanceData.OwnerChangedHandle);
	}

	InstanceData.OwnerChangedHandle.Reset();

	if (IsValid(InstanceData.CompanionActor))
	{
		if (UWorld* World = InstanceData.CompanionActor->GetWorld())
		{
			World->GetTimerManager().ClearTimer(InstanceData.FollowCheckTimer);
		}
	}

	InstanceData.FollowCheckTimer.Invalidate();
}

