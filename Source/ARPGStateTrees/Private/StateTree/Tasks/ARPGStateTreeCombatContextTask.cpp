// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreeCombatContextTask.h"

#include "AI/ARPGEnemyAIController.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Components/ARPGCombatantComponent.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "Types/ARPGGameplayTags.h"

FARPGStateTreeCombatContextTask::FARPGStateTreeCombatContextTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bConsideredForCompletion = false;
}

EStateTreeRunStatus FARPGStateTreeCombatContextTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.AIController))
	{
		return EStateTreeRunStatus::Failed;
	}

	const AARPGEnemyCharacter* EnemyCharacter = Cast<AARPGEnemyCharacter>(InstanceData.AIController->GetPawn());

	if (!IsValid(EnemyCharacter))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CombatantComponent = EnemyCharacter->GetCombatantComponent();

	if (!IsValid(InstanceData.CombatantComponent))
	{
		return EStateTreeRunStatus::Failed;
	}

	UpdateTarget(InstanceData);
	UpdateCoordination(InstanceData);
	UpdateAttackOpportunity(InstanceData);

	TStateTreeInstanceDataStructRef<FInstanceDataType> InstanceDataRef = Context.GetInstanceDataStructRef(*this);
	const FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();

	InstanceData.TargetChangedHandle = InstanceData.CombatantComponent->OnTargetChanged.AddLambda(
		[InstanceDataRef, WeakContext](UARPGCombatantComponent*, UARPGCombatantComponent*) mutable
		{
			FInstanceDataType* Data = InstanceDataRef.GetPtr();

			if (!Data)
			{
				return;
			}

			UpdateTarget(*Data);
			WeakContext.SendEvent(ARPGGameplayTags::StateTreeEvent_CombatTargetChanged, FConstStructView(), FName(TEXT("CombatTarget")));
		});

	InstanceData.CoordinationChangedHandle = InstanceData.CombatantComponent->OnCoordinationChanged.AddLambda(
		[InstanceDataRef, WeakContext]() mutable
		{
			FInstanceDataType* Data = InstanceDataRef.GetPtr();

			if (!Data)
			{
				return;
			}

			const EARPGCoordinationState PreviousState = Data->CoordinationState;
			const FVector PreviousLocation = Data->CoordinationLocation;

			UpdateCoordination(*Data);

			if (PreviousState != Data->CoordinationState)
			{
				WeakContext.SendEvent(ARPGGameplayTags::StateTreeEvent_CombatRoleChanged, FConstStructView(), FName(TEXT("CombatRole")));
				return;
			}

			if (!PreviousLocation.Equals(Data->CoordinationLocation, 1.0f))
			{
				WeakContext.SendEvent(ARPGGameplayTags::StateTreeEvent_CombatGoalChanged, FConstStructView(), FName(TEXT("CombatGoal")));
			}
		});
	
	InstanceData.AttackOpportunityChangedHandle = InstanceData.CombatantComponent->OnAttackOpportunityChanged.AddLambda(
	[InstanceDataRef, WeakContext](const bool bInRange) mutable
	{
		FInstanceDataType* Data = InstanceDataRef.GetPtr();

		if (!Data)
		{
			return;
		}

		Data->bTargetInAttackRange = bInRange;

		if (bInRange)
		{
			WeakContext.SendEvent(ARPGGameplayTags::StateTreeEvent_AttackOpportunity, FConstStructView(),FName(TEXT("AttackOpportunity")));
		}
	});

	return EStateTreeRunStatus::Running;
}

void FARPGStateTreeCombatContextTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (IsValid(InstanceData.CombatantComponent))
	{
		InstanceData.CombatantComponent->OnTargetChanged.Remove(InstanceData.TargetChangedHandle);
		InstanceData.CombatantComponent->OnCoordinationChanged.Remove(InstanceData.CoordinationChangedHandle);
		InstanceData.CombatantComponent->OnAttackOpportunityChanged.Remove(InstanceData.AttackOpportunityChangedHandle);
	}

	InstanceData.TargetChangedHandle.Reset();
	InstanceData.CoordinationChangedHandle.Reset();
	InstanceData.AttackOpportunityChangedHandle.Reset();
	
	InstanceData.CombatantComponent = nullptr;
}

void FARPGStateTreeCombatContextTask::UpdateTarget(FInstanceDataType& InstanceData)
{
	if (!IsValid(InstanceData.CombatantComponent))
	{
		InstanceData.TargetActor = nullptr;
		return;
	}

	const UARPGCombatantComponent* TargetCombatant = InstanceData.CombatantComponent->GetCurrentTarget();
	InstanceData.TargetActor = IsValid(TargetCombatant) ? TargetCombatant->GetCombatantActor() : nullptr;
}

void FARPGStateTreeCombatContextTask::UpdateCoordination(FInstanceDataType& InstanceData)
{
	if (!IsValid(InstanceData.CombatantComponent))
	{
		InstanceData.CoordinationState = EARPGCoordinationState::None;
		InstanceData.CoordinationLocation = FVector::ZeroVector;
		return;
	}

	InstanceData.CoordinationState = InstanceData.CombatantComponent->GetCoordinationState();
	InstanceData.CoordinationLocation = InstanceData.CombatantComponent->GetCoordinationLocation();
}

void FARPGStateTreeCombatContextTask::UpdateAttackOpportunity(FInstanceDataType& InstanceData)
{
	InstanceData.bTargetInAttackRange = IsValid(InstanceData.CombatantComponent) && InstanceData.CombatantComponent->IsTargetInAttackRange();
}
