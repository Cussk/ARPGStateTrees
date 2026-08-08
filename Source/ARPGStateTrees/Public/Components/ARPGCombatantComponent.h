// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/ARPGCombatTypes.h"
#include "ARPGCombatantComponent.generated.h"

class UARPGCombatantComponent;

DECLARE_MULTICAST_DELEGATE(FARPGCoordinationChanged);
DECLARE_MULTICAST_DELEGATE_TwoParams(FARPGCombatantTargetChanged, UARPGCombatantComponent*, UARPGCombatantComponent*);

/**
 * Provides shared combat identity, targeting, and coordination state for combatants.
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
	void SetCoordination(EARPGEngagementState NewState, const FVector& NewLocation);

	EARPGCombatTeam GetTeam() const;
	bool IsTargetable() const;
	bool IsHostileTo(const UARPGCombatantComponent* Other) const;

	EARPGEngagementState GetEngagementState() const;
	const FVector& GetEngagementLocation() const;

	float GetOccupancyRadius() const;
	float GetMaxEngagementDistance() const;
	int32 GetEngagementPriority() const;

	AActor* GetCombatantActor() const;
	UARPGCombatantComponent* GetCurrentTarget() const;

	FARPGCombatantTargetChanged OnTargetChanged;
	FARPGCoordinationChanged OnCoordinationChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EARPGCombatTeam Team = EARPGCombatTeam::Neutral;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	bool bTargetable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement")
	float OccupancyRadius = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement")
	float MaxEngagementDistance = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement")
	int32 EngagementPriority = 0;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CombatantActor;

	UPROPERTY(Transient)
	EARPGEngagementState EngagementState = EARPGEngagementState::None;

	UPROPERTY(Transient)
	FVector EngagementLocation = FVector::ZeroVector;

	TWeakObjectPtr<UARPGCombatantComponent> CurrentTarget;
};