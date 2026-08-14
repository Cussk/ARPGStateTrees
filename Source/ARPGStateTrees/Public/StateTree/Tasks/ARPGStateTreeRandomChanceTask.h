// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreeRandomChanceTask.generated.h"

USTRUCT()
struct FARPGStateTreeRandomChanceTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 0.25f;
};

USTRUCT(meta = (DisplayName = "ARPG Random Chance", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreeRandomChanceTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreeRandomChanceTaskInstanceData;

	FARPGStateTreeRandomChanceTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
