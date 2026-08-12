// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Types/ARPGCombatTypes.h"
#include "ARPGCombatCoordinationComponent.generated.h"

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
	bool HasCoordinatedAttackers(EARPGPositioningMode PositioningMode) const;

	void RequestMeleeFieldRefresh();
	void OnMeleeFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	void RequestRangedFieldRefresh();
	void OnRangedFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	void RebuildAssignments();

	void BuildEngagementAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;
	void BuildPressureAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;
	void BuildRangedAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;

	void CommitMeleeAssignments(const TArray<UARPGCombatantComponent*>& MeleeAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments);
	void CommitRangedAssignments(const TArray<UARPGCombatantComponent*>& RangedAttackers,
		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments);

	void UpdateMeleeAssignmentGoals();
	void UpdateRangedAssignmentGoals();

	int32 FindBestEngagementCandidate(const UARPGCombatantComponent* Attacker,
		const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments,
		bool bForceReposition = false, int32 ExistingCandidateIndex = INDEX_NONE) const;
	int32 FindBestPressureCandidate(const UARPGCombatantComponent* Attacker,
		const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments,
		bool bForceRetarget) const;
	int32 FindBestRangedCandidate(const UARPGCombatantComponent* Attacker,
		const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;

	bool CanOccupyMeleeCandidate(const UARPGCombatantComponent* Attacker, int32 CandidateIndex,
		const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;
	bool CanOccupyRangedCandidate(const UARPGCombatantComponent* Attacker, int32 CandidateIndex,
		const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const;

	bool IsPressureRetargetDue(const UARPGCombatantComponent* Attacker, double CurrentTime) const;
	void ScheduleNextPressureRetarget(UARPGCombatantComponent* Attacker, double CurrentTime);

	void HandleAttackerAttackCompleted(UARPGCombatantComponent* Attacker);

	void BindAttackerEvents(UARPGCombatantComponent* Attacker);
	void UnbindAttackerEvents(UARPGCombatantComponent* Attacker);

	void ResetEngagementAttackCount(UARPGCombatantComponent* Attacker);
	bool IsEngagementRepositionRequested(const UARPGCombatantComponent* Attacker) const;

	FVector GetMeleeCandidateWorldLocation(int32 CandidateIndex) const;
	FVector GetRangedCandidateWorldLocation(int32 CandidateIndex) const;

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
	float RangedCoordinationActivationRadius = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|Ranged")
	float RangedCoordinationDeactivationRadius = 1100.0f;

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

	TArray<FARPGCoordinationCandidate> MeleeCandidates;
	TArray<FARPGCoordinationCandidate> RangedCandidates;

	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> MeleeAssignments;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> RangedAssignments;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, double> NextPressureRetargetTimes;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, int32> RemainingAttacksBeforeReposition;
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FDelegateHandle> AttackCompletedDelegateHandles;

	FVector MeleeFieldOrigin = FVector::ZeroVector;
	FVector MeleePendingQueryOrigin = FVector::ZeroVector;
	FVector LastMeleeGoalUpdateOrigin = FVector::ZeroVector;

	FVector RangedFieldOrigin = FVector::ZeroVector;
	FVector RangedPendingQueryOrigin = FVector::ZeroVector;
	FVector LastRangedGoalUpdateOrigin = FVector::ZeroVector;

	FTimerHandle CoordinationTimer;

	int32 ActiveMeleeQueryId = INDEX_NONE;
	int32 ActiveRangedQueryId = INDEX_NONE;

	double LastAssignmentReevaluationTime = 0.0;

	bool bMeleeFieldInitialized = false;
	bool bRangedFieldInitialized = false;
	bool bMeleeAssignmentsDirty = false;
	bool bRangedAssignmentsDirty = false;
};
