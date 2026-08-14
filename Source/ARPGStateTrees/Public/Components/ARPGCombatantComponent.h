// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/ARPGCombatTypes.h"
#include "ARPGCombatantComponent.generated.h"

class UARPGCombatantComponent;

DECLARE_MULTICAST_DELEGATE(FARPGCoordinationChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FARPGCombatantAttackCompleted, UARPGCombatantComponent*);
DECLARE_MULTICAST_DELEGATE_OneParam(FARPGAttackOpportunityChanged, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FARPGCombatantSupportOpportunityChanged, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FARPGCombatantSupportAbilityUsed, UARPGCombatantComponent*);
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
	void SetCoordination(EARPGCoordinationState NewState, const FVector& NewLocation);
	void SetTargetInAttackRange(bool bInRange);

	EARPGCombatTeam GetTeam() const;
	bool IsTargetable() const;
	bool IsHostileTo(const UARPGCombatantComponent* Other) const;

	EARPGCoordinationState GetCoordinationState() const;
	const FVector& GetCoordinationLocation() const;

	float GetOccupancyRadius() const;
	float GetMaxEngagementDistance() const;
	int32 GetCoordinationPriority() const;

	float GetBasicAttackRange() const;
	bool IsTargetInAttackRange() const;

	AActor* GetCombatantActor() const;
	UARPGCombatantComponent* GetCurrentTarget() const;
	EARPGPositioningMode GetPositioningMode() const;
	
	void SetSupportOpportunity(bool bAvailable);
	bool HasSupportOpportunity() const;

	void NotifyAttackCompleted();
	void NotifySupportAbilityUsed();

	int32 GetMinAttacksBeforeReposition() const;
	int32 GetMaxAttacksBeforeReposition() const;

	FARPGCombatantTargetChanged OnTargetChanged;
	FARPGCombatantAttackCompleted OnAttackCompleted;
	FARPGAttackOpportunityChanged OnAttackOpportunityChanged;
	FARPGCombatantSupportOpportunityChanged OnSupportOpportunityChanged;
	FARPGCombatantSupportAbilityUsed OnSupportAbilityUsed;
	FARPGCoordinationChanged OnCoordinationChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EARPGCombatTeam Team = EARPGCombatTeam::Neutral;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Coordination")
    EARPGPositioningMode PositioningMode = EARPGPositioningMode::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	bool bTargetable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement")
	float OccupancyRadius = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement")
	float MaxEngagementDistance = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement")
	int32 CoordinationPriority = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement", meta = (ClampMin = "1"))
	int32 MinAttacksBeforeReposition = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Engagement", meta = (ClampMin = "1"))
	int32 MaxAttacksBeforeReposition = 4;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Attack", meta = (ClampMin = "0.0"))
	float BasicAttackRange = 165.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CombatantActor;

	UPROPERTY(Transient)
	EARPGCoordinationState CoordinationState = EARPGCoordinationState::None;

	UPROPERTY(Transient)
	FVector CoordinationLocation = FVector::ZeroVector;
	
	UPROPERTY(Transient)
	bool bTargetInAttackRange = false;
	
	UPROPERTY(Transient)
	bool bSupportOpportunity = false;

	TWeakObjectPtr<UARPGCombatantComponent> CurrentTarget;
};