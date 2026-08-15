// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/Object.h"
#include "ARPGCharacterBase.generated.h"

class UARPGCombatCoordinationComponent;
class UARPGCombatantComponent;
/**
 * 
 */
UCLASS()
class ARPGSTATETREES_API AARPGCharacterBase : public ACharacter
{
	GENERATED_BODY()
	
public:
	AARPGCharacterBase();
	
	UFUNCTION(BlueprintPure)
	UARPGCombatantComponent* GetCombatantComponent() const;
	
	UFUNCTION(BlueprintPure)
	UARPGCombatCoordinationComponent* GetCombatCoordinationComponent() const;
	
	float PlayMontage(UAnimMontage* Montage, FOnMontageBlendingOutStarted& BlendOutDelegate, float PlayRate = 1.0f) const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UARPGCombatCoordinationComponent> CombatCoordinationComponent;
};
