// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ARPGStateTreePlayMontageTask.generated.h"

class AARPGEnemyCharacter;
class UAnimInstance;
class UAnimMontage;

USTRUCT()
struct FARPGStateTreePlayMontageTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AARPGEnemyCharacter> Character;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> FacingTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> AnimInstance;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveMontage;

	bool bBlendOutStarted = false;
};

/**
 * Plays a montage and completes when the montage begins blending out.
 */
USTRUCT(meta = (DisplayName = "ARPG Play Montage", Category = "ARPG"))
struct ARPGSTATETREES_API FARPGStateTreePlayMontageTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FARPGStateTreePlayMontageTaskInstanceData;

	FARPGStateTreePlayMontageTask();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};