// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ARPGPlayerController.generated.h"

class UARPGNavigationComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class ARPGSTATETREES_API AARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void SetPawn(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveToAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0.01"))
	float HeldMoveUpdateInterval = 0.05f;

private:
	void OnMoveStarted(const FInputActionValue& InputValue);
	void OnMoveEnded(const FInputActionValue& InputValue);
	void UpdateHeldMove();
	void RequestCursorMove(bool bContinuousUpdate) const;
	bool ResolveCursorWorldLocation(FVector& OutWorldLocation) const;

	TWeakObjectPtr<UARPGNavigationComponent> NavigationComponent;
	FTimerHandle HeldMoveTimerHandle;
};