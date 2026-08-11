// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeCrowdRightOfWayTask.generated.h"

class AARPGEnemyCharacter;

USTRUCT()
struct FARPGStateTreeCrowdRightOfWayTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AARPGEnemyCharacter> Character;
};

USTRUCT(meta = (DisplayName = "ARPG Crowd Right Of Way", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeCrowdRightOfWayTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeCrowdRightOfWayTaskInstanceData;

	FARPGStateTreeCrowdRightOfWayTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};