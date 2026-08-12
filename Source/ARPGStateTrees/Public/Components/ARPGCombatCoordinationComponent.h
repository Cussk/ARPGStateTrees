// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Types/ARPGCombatTypes.h"
#include "ARPGCombatCoordinationComponent.generated.h"

class UARPGCrowdMovementComponent;
class UARPGCombatantComponent;
class UEnvQuery;

USTRUCT()
struct FARPGCoordinationCandidate
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
};

USTRUCT()
struct FARPGCombatAssignment
{
	GENERATED_BODY()

	EARPGCoordinationState State = EARPGCoordinationState::None;
	int32 CandidateIndex = INDEX_NONE;
};

/**
 * Coordinates local combat positioning around this component's owning combatant.
 */
UCLASS(ClassGroup = "ARPG", meta = (BlueprintSpawnableComponent))
class ARPGSTATETREES_API UARPGCombatCoordinationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGCombatCoordinationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RegisterAttacker(UARPGCombatantComponent* Attacker);
	void UnregisterAttacker(UARPGCombatantComponent* Attacker);

protected:
	void StartCoordination();
	void StopCoordination();
	void UpdateCoordination();

	bool UpdateCoordinatedAttackers();

	void RequestFieldRefresh();
	void OnFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	void RebuildAssignments();
	void BuildEngagementAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;
	void BuildPressureAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;
	void CommitAssignments(TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments);
	void UpdateAssignmentGoals();

	int32 FindBestEngagementCandidate(const UARPGCombatantComponent* Attacker, const TMap<TWeakObjectPtr<UARPGCombatantComponent>, 
		FARPGCombatAssignment>& PendingAssignments, bool bForceReposition = false, int32 ExistingCandidateIndex = INDEX_NONE) const;
	int32 FindBestPressureCandidate(const UARPGCombatantComponent* Attacker,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments, bool bForceRetarget) const;

	bool CanOccupyCandidate(const UARPGCombatantComponent* Attacker, int32 CandidateIndex,
		const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;
	
	bool IsPressureRetargetDue(const UARPGCombatantComponent* Attacker, double CurrentTime) const;
	void ScheduleNextPressureRetarget(UARPGCombatantComponent* Attacker, double CurrentTime);
	
	void HandleAttackerAttackCompleted(UARPGCombatantComponent* Attacker);

	void BindAttackerEvents(UARPGCombatantComponent* Attacker);
	void UnbindAttackerEvents(UARPGCombatantComponent* Attacker);

	void ResetEngagementAttackCount(UARPGCombatantComponent* Attacker);
	bool IsEngagementRepositionRequested(const UARPGCombatantComponent* Attacker) const;

	FVector GetCandidateWorldLocation(int32 CandidateIndex) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|EQS")
	TObjectPtr<UEnvQuery> MeleeEngagementQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination", meta = (ClampMin = "0.05"))
	float CoordinationInterval = 0.10f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float CoordinationActivationRadius = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float CoordinationDeactivationRadius = 850.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float AssignmentGoalUpdateDistance = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float FieldRefreshDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float PressureMinDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float AssignmentSeparation = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination", meta = (ClampMin = "0.05"))
	float AssignmentReevaluationInterval = 0.35f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Engagement", meta = (ClampMin = "0.0"))
	float EngagementRepositionMinMoveDistance = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Pressure", meta = (ClampMin = "0.05"))
	float PressureRetargetMinInterval = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Pressure", meta = (ClampMin = "0.05"))
	float PressureRetargetMaxInterval = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Pressure", meta = (ClampMin = "0.0"))
	float PressureRetargetMinMoveDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Pressure", meta = (ClampMin = "0.0"))
	float PressureRadialWeight = 2.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Ranged")
	TObjectPtr<UEnvQuery> RangedCoordinationQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Ranged")
	float RangedMinDistance = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Ranged")
	float RangedMaxDistance = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Ranged")
	float RangedFieldRefreshDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Ranged")
	float RangedGoalUpdateDistance = 50.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	TSet<TWeakObjectPtr<UARPGCombatantComponent>> Attackers;
	TSet<TWeakObjectPtr<UARPGCombatantComponent>> CoordinatedAttackers;
	TSet<TWeakObjectPtr<UARPGCombatantComponent>> EngagementRepositionRequests;

	TArray<FARPGCoordinationCandidate> Candidates;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> MeleeAssignments;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> RangedAssignments;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, double> NextPressureRetargetTimes;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, int32> RemainingAttacksBeforeReposition;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FDelegateHandle> AttackCompletedDelegateHandles;

	FVector FieldOrigin = FVector::ZeroVector;
	FVector PendingQueryOrigin = FVector::ZeroVector;
	FVector LastGoalUpdateOrigin = FVector::ZeroVector;

	FTimerHandle CoordinationTimer;

	int32 ActiveQueryId = INDEX_NONE;
	
	float LastAssignmentReevaluationTime = 0.0;

	bool bFieldInitialized = false;
	bool bAssignmentsDirty = false;
};