// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGEnemyCharacter.generated.h"

class UARPGCombatantComponent;
class UStateTree;

UCLASS()
class ARPGSTATETREES_API AARPGEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AARPGEnemyCharacter();

	UFUNCTION(BlueprintPure)
	UARPGCombatantComponent* GetCombatantComponent() const;
	
	UFUNCTION(BlueprintPure)
	UStateTree* GetStateTreeAsset() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTreeAsset;
};