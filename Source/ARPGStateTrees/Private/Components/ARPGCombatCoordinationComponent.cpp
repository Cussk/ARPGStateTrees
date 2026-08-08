// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/ARPGCombatCoordinationComponent.h"

#include "Components/ARPGCombatantComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UARPGCombatCoordinationComponent::UARPGCombatCoordinationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGCombatCoordinationComponent::BeginPlay()
{
	Super::BeginPlay();

	TargetActor = GetOwner();
}

void UARPGCombatCoordinationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCoordination();

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : Attackers)
	{
		if (UARPGCombatantComponent* Attacker = WeakAttacker.Get())
		{
			Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);
		}
	}

	Attackers.Reset();
	CoordinatedAttackers.Reset();
	Assignments.Reset();

	Super::EndPlay(EndPlayReason);
}

void UARPGCombatCoordinationComponent::RegisterAttacker(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker) || Attackers.Contains(Attacker))
	{
		return;
	}

	Attackers.Add(Attacker);
	bAssignmentsDirty = true;

	if (Attackers.Num() == 1)
	{
		StartCoordination();
	}
}

void UARPGCombatCoordinationComponent::UnregisterAttacker(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker) || Attackers.Remove(Attacker) == 0)
	{
		return;
	}

	CoordinatedAttackers.Remove(Attacker);
	Assignments.Remove(Attacker);

	Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);

	bAssignmentsDirty = true;

	if (Attackers.IsEmpty())
	{
		StopCoordination();
	}
}

void UARPGCombatCoordinationComponent::StartCoordination()
{
	UpdateCoordination();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoordinationTimer, this, &UARPGCombatCoordinationComponent::UpdateCoordination,
			CoordinationInterval, true);
	}
}

void UARPGCombatCoordinationComponent::StopCoordination()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoordinationTimer);
	}

	if (ActiveQueryId != INDEX_NONE)
	{
		if (UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(this))
		{
			QueryManager->AbortQuery(ActiveQueryId);
		}

		ActiveQueryId = INDEX_NONE;
	}

	Candidates.Reset();
	Assignments.Reset();
	CoordinatedAttackers.Reset();

	bFieldInitialized = false;
	bAssignmentsDirty = false;
}

void UARPGCombatCoordinationComponent::UpdateCoordination()
{
	if (!IsValid(TargetActor) || Attackers.IsEmpty())
	{
		return;
	}

	if (UpdateCoordinatedAttackers())
	{
		bAssignmentsDirty = true;
	}

	if (Attackers.IsEmpty())
	{
		StopCoordination();
		return;
	}

	if (CoordinatedAttackers.IsEmpty())
	{
		return;
	}

	if (!bFieldInitialized)
	{
		RequestFieldRefresh();
		return;
	}

	UpdateAssignmentGoals();

	const bool bFieldStale = FVector::DistSquared2D(TargetActor->GetActorLocation(), FieldOrigin)
		>= FMath::Square(FieldRefreshDistance);

	if (bFieldStale)
	{
		RequestFieldRefresh();
	}

	if (bAssignmentsDirty && !bFieldStale)
	{
		RebuildAssignments();
	}
}

bool UARPGCombatCoordinationComponent::UpdateCoordinatedAttackers()
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	bool bChanged = false;

	for (auto Iterator = Attackers.CreateIterator(); Iterator; ++Iterator)
	{
		UARPGCombatantComponent* Attacker = Iterator->Get();

		if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()))
		{
			CoordinatedAttackers.Remove(*Iterator);
			Assignments.Remove(*Iterator);
			Iterator.RemoveCurrent();
			bChanged = true;
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(TargetLocation, Attacker->GetCombatantActor()->GetActorLocation());
		const bool bIsCoordinated = CoordinatedAttackers.Contains(Attacker);

		if (bIsCoordinated)
		{
			if (DistanceSquared > FMath::Square(CoordinationDeactivationRadius))
			{
				CoordinatedAttackers.Remove(Attacker);
				Assignments.Remove(Attacker);
				Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);
				bChanged = true;
			}

			continue;
		}

		if (DistanceSquared <= FMath::Square(CoordinationActivationRadius))
		{
			CoordinatedAttackers.Add(Attacker);
			bChanged = true;
		}
	}

	return bChanged;
}

void UARPGCombatCoordinationComponent::RequestFieldRefresh()
{
	if (!IsValid(EngagementQuery) || !IsValid(TargetActor) || ActiveQueryId != INDEX_NONE)
	{
		return;
	}

	PendingQueryOrigin = TargetActor->GetActorLocation();

	FEnvQueryRequest QueryRequest(EngagementQuery, TargetActor);
	ActiveQueryId = QueryRequest.Execute(EEnvQueryRunMode::AllMatching, this, &UARPGCombatCoordinationComponent::OnFieldQueryFinished);
}

void UARPGCombatCoordinationComponent::OnFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	ActiveQueryId = INDEX_NONE;

	if (!Result.IsValid() || !Result->IsSuccessful() || !IsValid(TargetActor) || Attackers.IsEmpty())
	{
		return;
	}

	Candidates.Reset();
	Candidates.Reserve(Result->Items.Num());

	for (int32 Index = 0; Index < Result->Items.Num(); ++Index)
	{
		FARPGCoordinationCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Location = Result->GetItemAsLocation(Index);
	}

	FieldOrigin = PendingQueryOrigin;
	bFieldInitialized = true;
	bAssignmentsDirty = true;

	RebuildAssignments();
}

void UARPGCombatCoordinationComponent::RebuildAssignments()
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	TArray<UARPGCombatantComponent*> SortedAttackers;
	SortedAttackers.Reserve(CoordinatedAttackers.Num());

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : CoordinatedAttackers)
	{
		if (UARPGCombatantComponent* Attacker = WeakAttacker.Get())
		{
			SortedAttackers.Add(Attacker);
		}
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	SortedAttackers.Sort([TargetLocation](const UARPGCombatantComponent& A, const UARPGCombatantComponent& B)
	{
		if (A.GetEngagementPriority() != B.GetEngagementPriority())
		{
			return A.GetEngagementPriority() > B.GetEngagementPriority();
		}

		const float DistanceA = FVector::DistSquared2D(A.GetCombatantActor()->GetActorLocation(), TargetLocation);
		const float DistanceB = FVector::DistSquared2D(B.GetCombatantActor()->GetActorLocation(), TargetLocation);

		return DistanceA < DistanceB;
	});

	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> PendingAssignments;

	if (!Candidates.IsEmpty())
	{
		BuildEngagementAssignments(SortedAttackers, PendingAssignments);
		BuildPressureAssignments(SortedAttackers, PendingAssignments);
	}

	CommitAssignments(PendingAssignments);

	LastGoalUpdateOrigin = TargetLocation;
	bAssignmentsDirty = false;
}

void UARPGCombatCoordinationComponent::BuildEngagementAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		const int32 CandidateIndex = FindBestEngagementCandidate(Attacker, PendingAssignments);

		if (CandidateIndex == INDEX_NONE)
		{
			continue;
		}

		FARPGCombatAssignment Assignment;
		Assignment.State = EARPGEngagementState::Engaged;
		Assignment.CandidateIndex = CandidateIndex;

		PendingAssignments.Add(Attacker, Assignment);
	}
}

void UARPGCombatCoordinationComponent::BuildPressureAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		if (PendingAssignments.Contains(Attacker))
		{
			continue;
		}

		const int32 CandidateIndex = FindBestPressureCandidate(Attacker, PendingAssignments);

		if (CandidateIndex == INDEX_NONE)
		{
			continue;
		}

		FARPGCombatAssignment Assignment;
		Assignment.State = EARPGEngagementState::Pressure;
		Assignment.CandidateIndex = CandidateIndex;

		PendingAssignments.Add(Attacker, Assignment);
	}
}

void UARPGCombatCoordinationComponent::CommitAssignments(
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments)
{
	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : CoordinatedAttackers)
	{
		UARPGCombatantComponent* Attacker = WeakAttacker.Get();

		if (!IsValid(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* Assignment = PendingAssignments.Find(WeakAttacker);

		if (!Assignment || !Candidates.IsValidIndex(Assignment->CandidateIndex))
		{
			Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);
			continue;
		}

		Attacker->SetCoordination(Assignment->State, GetCandidateWorldLocation(Assignment->CandidateIndex));
	}

	Assignments = MoveTemp(PendingAssignments);
}

void UARPGCombatCoordinationComponent::UpdateAssignmentGoals()
{
	if (!IsValid(TargetActor) || Assignments.IsEmpty())
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	if (FVector::DistSquared2D(TargetLocation, LastGoalUpdateOrigin) < FMath::Square(AssignmentGoalUpdateDistance))
	{
		return;
	}

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : Assignments)
	{
		UARPGCombatantComponent* Attacker = Pair.Key.Get();

		if (!IsValid(Attacker) || !Candidates.IsValidIndex(Pair.Value.CandidateIndex))
		{
			continue;
		}

		Attacker->SetCoordination(Pair.Value.State, GetCandidateWorldLocation(Pair.Value.CandidateIndex));
	}

	LastGoalUpdateOrigin = TargetLocation;
}

int32 UARPGCombatCoordinationComponent::FindBestEngagementCandidate(const UARPGCombatantComponent* Attacker,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()) || !IsValid(TargetActor))
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = FLT_MAX;

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FVector CandidateLocation = GetCandidateWorldLocation(Index);

		if (FVector::DistSquared2D(TargetLocation, CandidateLocation) > FMath::Square(Attacker->GetMaxEngagementDistance()))
		{
			continue;
		}

		if (!CanOccupyCandidate(Attacker, Index, PendingAssignments))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(AttackerLocation, CandidateLocation);

		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		BestIndex = Index;
	}

	return BestIndex;
}

int32 UARPGCombatCoordinationComponent::FindBestPressureCandidate(const UARPGCombatantComponent* Attacker,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()) || !IsValid(TargetActor))
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = FLT_MAX;

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FVector CandidateLocation = GetCandidateWorldLocation(Index);

		if (FVector::DistSquared2D(TargetLocation, CandidateLocation) < FMath::Square(PressureMinDistance))
		{
			continue;
		}

		if (!CanOccupyCandidate(Attacker, Index, PendingAssignments))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(AttackerLocation, CandidateLocation);

		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		BestIndex = Index;
	}

	return BestIndex;
}

bool UARPGCombatCoordinationComponent::CanOccupyCandidate(const UARPGCombatantComponent* Attacker, const int32 CandidateIndex,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	if (!IsValid(Attacker) || !Candidates.IsValidIndex(CandidateIndex))
	{
		return false;
	}

	const FVector CandidateLocation = GetCandidateWorldLocation(CandidateIndex);

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : PendingAssignments)
	{
		const UARPGCombatantComponent* Other = Pair.Key.Get();

		if (!IsValid(Other) || !Candidates.IsValidIndex(Pair.Value.CandidateIndex))
		{
			continue;
		}

		const float RequiredSeparation = Attacker->GetOccupancyRadius() + Other->GetOccupancyRadius() + AssignmentSeparation;
		const FVector OtherLocation = GetCandidateWorldLocation(Pair.Value.CandidateIndex);

		if (FVector::DistSquared2D(CandidateLocation, OtherLocation) < FMath::Square(RequiredSeparation))
		{
			return false;
		}
	}

	return true;
}

FVector UARPGCombatCoordinationComponent::GetCandidateWorldLocation(const int32 CandidateIndex) const
{
	if (!Candidates.IsValidIndex(CandidateIndex) || !IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	FVector TargetDelta = TargetActor->GetActorLocation() - FieldOrigin;
	TargetDelta.Z = 0.0f;

	return Candidates[CandidateIndex].Location + TargetDelta;
}