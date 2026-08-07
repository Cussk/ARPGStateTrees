// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class ARPGSTATETREES_API AARPGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AARPGCharacter();

protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY(EditAnywhere, Category = "Camera")
	USpringArmComponent* SpringArmComponent;
	
	UPROPERTY(EditAnywhere, Category = "Camera")
	UCameraComponent* CameraComponent;
};
