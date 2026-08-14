// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeNotifySupportAbilityUsedTask.generated.h"

class UARPGCombatantComponent;

USTRUCT()
struct FARPGStateTreeNotifySupportAbilityUsedTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;
};

USTRUCT(meta = (DisplayName = "ARPG Notify Support Ability Used", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeNotifySupportAbilityUsedTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeNotifySupportAbilityUsedTaskInstanceData;

	FARPGStateTreeNotifySupportAbilityUsedTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
