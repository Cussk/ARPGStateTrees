// Copyright Kyle Cuss and Cuss Programming 2026.

#include "StateTree/Tasks/ARPGStateTreePlayMontageTask.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

FARPGStateTreePlayMontageTask::FARPGStateTreePlayMontageTask()
{
	bShouldCallTick = false;
	bShouldCallTickOnlyOnEvents = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FARPGStateTreePlayMontageTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Character) || !IsValid(InstanceData.Montage))
	{
		return EStateTreeRunStatus::Failed;
	}

	USkeletalMeshComponent* Mesh = InstanceData.Character->GetMesh();

	if (!IsValid(Mesh))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.AnimInstance = Mesh->GetAnimInstance();
	InstanceData.ActiveMontage = InstanceData.Montage;
	InstanceData.bBlendOutStarted = false;

	if (!IsValid(InstanceData.AnimInstance))
	{
		return EStateTreeRunStatus::Failed;
	}

	const FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();
	TStateTreeInstanceDataStructRef<FInstanceDataType> InstanceDataRef = Context.GetInstanceDataStructRef(*this);

	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindLambda([WeakContext, InstanceDataRef](UAnimMontage* Montage, const bool bInterrupted) mutable
	{
		FInstanceDataType* Data = InstanceDataRef.GetPtr();

		if (!Data || Montage != Data->ActiveMontage)
		{
			return;
		}

		Data->bBlendOutStarted = true;

		WeakContext.FinishTask(bInterrupted ? EStateTreeFinishTaskType::Failed : EStateTreeFinishTaskType::Succeeded);
	});

	const float Duration = InstanceData.Character->PlayMontage(InstanceData.ActiveMontage, BlendOutDelegate, InstanceData.PlayRate);

	if (Duration <= 0.0f)
	{
		InstanceData.AnimInstance = nullptr;
		InstanceData.ActiveMontage = nullptr;

		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FARPGStateTreePlayMontageTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (IsValid(InstanceData.AnimInstance) && IsValid(InstanceData.ActiveMontage))
	{
		FOnMontageBlendingOutStarted EmptyDelegate;
		InstanceData.AnimInstance->Montage_SetBlendingOutDelegate(EmptyDelegate, InstanceData.ActiveMontage);

		if (!InstanceData.bBlendOutStarted && InstanceData.AnimInstance->Montage_IsPlaying(InstanceData.ActiveMontage))
		{
			InstanceData.AnimInstance->Montage_Stop(0.1f, InstanceData.ActiveMontage);
		}
	}

	InstanceData.AnimInstance = nullptr;
	InstanceData.ActiveMontage = nullptr;
	InstanceData.bBlendOutStarted = false;
}