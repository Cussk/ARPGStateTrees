// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/ARPGCombatTypes.h"
#include "ARPGCombatantComponent.generated.h"

class UARPGCombatantComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FARPGCombatantTargetChanged, UARPGCombatantComponent*, UARPGCombatantComponent*);

/**
 * Provides shared combat identity and targeting state for players, enemies, and allied combatants.
 */
UCLASS(ClassGroup = "ARPG", meta = (BlueprintSpawnableComponent))
class ARPGSTATETREES_API UARPGCombatantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGCombatantComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetTeam(EARPGCombatTeam NewTeam);
	void SetTargetable(bool bNewTargetable);
	void SetCurrentTarget(UARPGCombatantComponent* NewTarget);

	EARPGCombatTeam GetTeam() const;
	bool IsTargetable() const;
	bool IsHostileTo(const UARPGCombatantComponent* Other) const;

	AActor* GetCombatantActor() const;
	UARPGCombatantComponent* GetCurrentTarget() const;

	FARPGCombatantTargetChanged OnTargetChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EARPGCombatTeam Team = EARPGCombatTeam::Neutral;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	bool bTargetable = true;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CombatantActor;

	TWeakObjectPtr<UARPGCombatantComponent> CurrentTarget;
};