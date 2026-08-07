// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGNavigationComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/**
 * Owns high-level navigation for a locally controlled ARPG character.
 *
 * Resolves requested destinations into navigation paths and converts those
 * paths into movement input consumed by UCharacterMovementComponent.
 *
 * This component does not own physical locomotion or network prediction.
 */
UCLASS(ClassGroup = "ARPG", meta = (BlueprintSpawnableComponent))
class ARPGSTATETREES_API UARPGNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGNavigationComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, Category = "ARPG|Navigation")
	bool RequestMoveToLocation(const FVector& WorldDestination);
	
	UFUNCTION(BlueprintCallable, Category = "ARPG|Navigation")
	bool UpdateMoveDestination(const FVector& WorldDestination);
	
	UFUNCTION(BlueprintCallable, Category = "ARPG|Navigation")
	void CancelNavigation();

	UFUNCTION(BlueprintPure, Category = "ARPG|Navigation")
	bool IsNavigating() const;

	FVector GetResolvedDestination() const { return ResolvedDestination; }

private:
	bool TryBuildPath(const FVector& WorldDestination);
	bool ShouldUpdateDestination(const FVector& WorldDestination) const;

	void RecordDestinationRequest(const FVector& WorldDestination);
	void AdvancePathIfNeeded();
	void CompleteNavigation();

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Path Following", meta = (ClampMin = "1.0"))
	float IntermediatePointAcceptanceRadius = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Path Following", meta = (ClampMin = "1.0"))
	float FinalDestinationAcceptanceRadius = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Path Finding")
	FVector NavigationProjectionExtent = FVector(100.0f, 100.0f, 250.0f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Continuous Update", meta = (ClampMin = "0.0"))
	float MinimumDestinationUpdateInterval = 0.10f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Continuous Update", meta = (ClampMin = "0.0"))
	float DestinationChangeThreshold = 75.0f;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent;

	TArray<FVector> PathPoints;

	int32 CurrentPathPointIndex = INDEX_NONE;

	FVector LastRequestedDestination = FVector::ZeroVector;
	FVector ResolvedDestination = FVector::ZeroVector;

	double LastDestinationRequestTime = 0.0;

	bool bHasDestinationRequest = false;
};