// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ARPGCombatCoordinationComponent.generated.h"

class UARPGCombatantComponent;
class UEnvQuery;

USTRUCT()
struct FARPGCoordinationCandidate
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	float Score = 0.0f;
};

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

	void RequestFieldRefresh();
	void OnFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	void RebuildAssignments();
	void BuildEngagementAssignments(TArray<UARPGCombatantComponent*>& SortedAttackers, TSet<int32>& OccupiedCandidates);
	void BuildPressureAssignments(const TArray<UARPGCombatantComponent*>& Attackers, const TSet<int32>& OccupiedCandidates);
	void UpdateAttackPermissions(const TArray<UARPGCombatantComponent*>& Attackers);

	int32 FindBestEngagementCandidate(const UARPGCombatantComponent* Attacker, const TSet<int32>& OccupiedCandidates) const;
	int32 FindBestPressureCandidate(const UARPGCombatantComponent* Attacker, const TSet<int32>& OccupiedCandidates) const;

	bool CanOccupyCandidate(const UARPGCombatantComponent* Attacker, int32 CandidateIndex) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination|EQS")
	TObjectPtr<UEnvQuery> EngagementQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination", meta = (ClampMin = "0.05"))
	float CoordinationInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float FieldRefreshDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float PressureMinDistance = 275.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	float AssignmentSeparation = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coordination")
	int32 MaxConcurrentAttackers = 3;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	TSet<TWeakObjectPtr<UARPGCombatantComponent>> Attackers;
	TArray<FARPGCoordinationCandidate> Candidates;

	FVector LastQueryOrigin = FVector::ZeroVector;

	FTimerHandle CoordinationTimer;
	int32 ActiveQueryId = INDEX_NONE;
	int32 AttackSelectionOffset = 0;

	bool bFieldInitialized = false;
	bool bAssignmentsDirty = false;
};