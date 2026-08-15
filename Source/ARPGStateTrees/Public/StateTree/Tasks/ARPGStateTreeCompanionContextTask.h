// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeCompanionContextTask.generated.h"

class UARPGCompanionComponent;

USTRUCT()
struct FARPGStateTreeCompanionContextTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> CompanionActor;

	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<UARPGCompanionComponent> CompanionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> CompanionOwnerActor;

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bShouldFollow = false;

	FTimerHandle FollowCheckTimer;
	FDelegateHandle OwnerChangedHandle;

	bool bActive = false;
};

USTRUCT(meta = (DisplayName = "ARPG Companion Context", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeCompanionContextTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeCompanionContextTaskInstanceData;

	FARPGStateTreeCompanionContextTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};
