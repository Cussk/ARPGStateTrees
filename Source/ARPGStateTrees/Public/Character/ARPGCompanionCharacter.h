// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "ARPGCompanionCharacter.generated.h"

class UStateTree;
class UARPGCompanionComponent;

UCLASS()
class ARPGSTATETREES_API AARPGCompanionCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AARPGCompanionCharacter();
	
	UFUNCTION(BlueprintPure)
	UStateTree* GetStateTreeAsset() const;
	
	UFUNCTION(BlueprintPure, Category = "Companion")
	UARPGCompanionComponent* GetCompanionComponent() const; 
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<UARPGCompanionComponent> CompanionComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTreeAsset;
};
