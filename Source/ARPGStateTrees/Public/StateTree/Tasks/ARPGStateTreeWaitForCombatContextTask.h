// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeWaitForCombatContextTask.generated.h"

USTRUCT()
struct FARPGStateTreeWaitForCombatContextTaskInstanceData
{
	GENERATED_BODY()
};

/**
 * Keeps a state active until a combat-context event causes a transition.
 */
USTRUCT(DisplayName = "ARPG Wait For Combat Context", Category = "ARPG")
struct ARPGSTATETREES_API FARPGStateTreeWaitForCombatContextTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeWaitForCombatContextTaskInstanceData;

	FARPGStateTreeWaitForCombatContextTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};