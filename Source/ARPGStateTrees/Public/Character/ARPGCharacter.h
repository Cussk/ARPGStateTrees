// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGCharacter.generated.h"

class UARPGCombatCoordinationComponent;
class UARPGCombatantComponent;
class UARPGNavigationComponent;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class ARPGSTATETREES_API AARPGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AARPGCharacter();
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintPure)
	UARPGNavigationComponent* GetARPGNavigationComponent() const;
	
	UFUNCTION(BlueprintPure)
	UARPGCombatantComponent* GetCombatantComponent() const;
	
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
	TObjectPtr<UARPGCombatantComponent> CombatantComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UARPGCombatCoordinationComponent> CombatCoordinationComponent;
};
