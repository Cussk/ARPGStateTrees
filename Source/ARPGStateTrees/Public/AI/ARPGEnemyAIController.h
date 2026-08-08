// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ARPGEnemyAIController.generated.h"

class UARPGCombatCoordinationComponent;
class UARPGCombatantComponent;
class UARPGCombatantRegistrySubsystem;
class UStateTreeAIComponent;

UCLASS()
class ARPGSTATETREES_API AARPGEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AARPGEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void UpdateTarget();
	void SetCurrentTarget(UARPGCombatantComponent* NewTarget);
	bool IsCurrentTargetValid() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Targeting")
	float TargetAcquisitionRadius = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Targeting")
	float TargetDropRadius = 3500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Targeting", meta = (ClampMin = "0.1"))
	float TargetRefreshInterval = 0.75f;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatantComponent> ControlledCombatantComponent;

	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatantRegistrySubsystem> CombatantRegistrySubsystem;
	
	UPROPERTY(Transient)
	TObjectPtr<UARPGCombatCoordinationComponent> TargetCoordinationComponent;

	TWeakObjectPtr<UARPGCombatantComponent> CurrentTargetCombatant;
	FTimerHandle TargetRefreshTimerHandle;
};