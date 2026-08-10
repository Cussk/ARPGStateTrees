// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "ARPGEnemyCharacter.generated.h"

class UARPGCombatantComponent;
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

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTreeAsset;
};