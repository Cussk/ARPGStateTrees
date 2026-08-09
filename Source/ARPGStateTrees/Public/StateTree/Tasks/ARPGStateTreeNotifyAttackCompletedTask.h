// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeNotifyAttackCompletedTask.generated.h"

class UARPGCombatantComponent;

USTRUCT()
struct FARPGStateTreeNotifyAttackCompletedTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;
};

USTRUCT(meta = (DisplayName = "ARPG Notify Attack Completed", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeNotifyAttackCompletedTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeNotifyAttackCompletedTaskInstanceData;

	FARPGStateTreeNotifyAttackCompletedTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};