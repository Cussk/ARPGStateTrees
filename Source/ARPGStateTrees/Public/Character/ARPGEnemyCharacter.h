// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "ARPGEnemyCharacter.generated.h"

class UARPGCrowdMovementComponent;
class UStateTree;
class UAnimMontage;

UCLASS()
class ARPGSTATETREES_API AARPGEnemyCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AARPGEnemyCharacter();
	
	UFUNCTION(BlueprintPure)
	UStateTree* GetStateTreeAsset() const;
	
	UFUNCTION(BlueprintPure)
	UARPGCrowdMovementComponent* GetCrowdMovementComponent() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTreeAsset;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UARPGCrowdMovementComponent> CrowdMovementComponent;
};