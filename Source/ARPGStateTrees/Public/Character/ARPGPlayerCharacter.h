// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Character/ARPGCharacterBase.h"
#include "ARPGPlayerCharacter.generated.h"

class UARPGCombatCoordinationComponent;
class UARPGNavigationComponent;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class ARPGSTATETREES_API AARPGPlayerCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AARPGPlayerCharacter();
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintPure)
	UARPGNavigationComponent* GetARPGNavigationComponent() const;
	
	UFUNCTION(BlueprintPure)
	UARPGCombatCoordinationComponent* GetCombatCoordinationComponent() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TObjectPtr<UARPGNavigationComponent> NavigationComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UARPGCombatCoordinationComponent> CombatCoordinationComponent;
};
