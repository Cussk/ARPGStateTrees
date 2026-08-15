// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ARPGCompanionAIController.generated.h"

class UARPGCombatantComponent;
class UARPGCombatCoordinationComponent;
class UARPGCombatantRegistrySubsystem;
class UARPGCompanionComponent;
class UStateTreeAIComponent;

UCLASS()
class ARPGSTATETREES_API AARPGCompanionAIController : public AAIController
{
	GENERATED_BODY()

public:
	AARPGCompanionAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void UpdateTarget();
	void SetCurrentTarget(UARPGCombatantComponent* NewTarget);
	bool IsCurrentTargetValid() const;
	bool IsWithinOwnerLeash(const AActor* Actor) const;
	void HandleCompanionOwnerChanged(AActor* NewOwner);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Targeting")
	float TargetAcquisitionRadius = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Targeting")
	float TargetDropRadius = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Targeting", meta = (ClampMin = "0.1"))
	float TargetRefreshInterval = 0.5f;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatantComponent> ControlledCombatantComponent;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCompanionComponent> CompanionComponent;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatantRegistrySubsystem> CombatantRegistrySubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatCoordinationComponent> TargetCoordinationComponent;

	TWeakObjectPtr<UARPGCombatantComponent> CurrentTargetCombatant;

	FDelegateHandle OwnerChangedHandle;
	FTimerHandle TargetRefreshTimerHandle;
};