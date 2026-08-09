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
			UnbindAttackerEvents(Attacker);
			Attacker->SetTargetInAttackRange(false);
			Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);
		}
	}

	AttackCompletedDelegateHandles.Reset();

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
	BindAttackerEvents(Attacker);

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

	UnbindAttackerEvents(Attacker);

	CoordinatedAttackers.Remove(Attacker);
	Assignments.Remove(Attacker);
	NextPressureRetargetTimes.Remove(Attacker);
	RemainingAttacksBeforeReposition.Remove(Attacker);
	EngagementRepositionRequests.Remove(Attacker);

	Attacker->SetTargetInAttackRange(false);
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
	NextPressureRetargetTimes.Reset();
	RemainingAttacksBeforeReposition.Reset();
	EngagementRepositionRequests.Reset();
	LastAssignmentReevaluationTime = 0.0;

	bFieldInitialized = false;
	bAssignmentsDirty = false;
}

void UARPGCombatCoordinationComponent::UpdateCoordination()
{
	if (!IsValid(TargetActor) || Attackers.IsEmpty())
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!World)
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

	const double CurrentTime = World->GetTimeSeconds();
	const bool bReevaluationDue = CurrentTime - LastAssignmentReevaluationTime >= AssignmentReevaluationInterval;

	if ((bAssignmentsDirty || bReevaluationDue) && !bFieldStale)
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
			NextPressureRetargetTimes.Remove(*Iterator);
			Iterator.RemoveCurrent();
			bChanged = true;			
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(TargetLocation, Attacker->GetCombatantActor()->GetActorLocation());

		const bool bTargetInAttackRange = DistanceSquared <= FMath::Square(Attacker->GetBasicAttackRange());
		Attacker->SetTargetInAttackRange(bTargetInAttackRange);

		const bool bIsCoordinated = CoordinatedAttackers.Contains(Attacker);

		if (bIsCoordinated)
		{
			if (DistanceSquared > FMath::Square(CoordinationDeactivationRadius))
			{
				CoordinatedAttackers.Remove(Attacker);
				Assignments.Remove(Attacker);
				NextPressureRetargetTimes.Remove(Attacker);
				RemainingAttacksBeforeReposition.Remove(Attacker);
				EngagementRepositionRequests.Remove(Attacker);
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

	SortedAttackers.Sort([this, TargetLocation](const UARPGCombatantComponent& A, const UARPGCombatantComponent& B)
	{
		if (A.GetEngagementPriority() != B.GetEngagementPriority())
		{
			return A.GetEngagementPriority() > B.GetEngagementPriority();
		}

		const bool bAEngaged = A.GetEngagementState() == EARPGEngagementState::Engaged;
		const bool bBEngaged = B.GetEngagementState() == EARPGEngagementState::Engaged;

		if (bAEngaged != bBEngaged)
		{
			return bAEngaged;
		}

		if (bAEngaged && bBEngaged)
		{
			const bool bARepositioning = IsEngagementRepositionRequested(&A);
			const bool bBRepositioning = IsEngagementRepositionRequested(&B);

			if (bARepositioning != bBRepositioning)
			{
				return !bARepositioning;
			}
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

	if (const UWorld* World = GetWorld())
	{
		LastAssignmentReevaluationTime = World->GetTimeSeconds();
	}

	LastGoalUpdateOrigin = TargetLocation;
	bAssignmentsDirty = false;
}

void UARPGCombatCoordinationComponent::BuildEngagementAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		int32 CandidateIndex = INDEX_NONE;

		const FARPGCombatAssignment* ExistingAssignment = Assignments.Find(Attacker);
		const bool bHasExistingEngagement = ExistingAssignment
			&& ExistingAssignment->State == EARPGEngagementState::Engaged
			&& Candidates.IsValidIndex(ExistingAssignment->CandidateIndex);

		const bool bRepositionRequested = IsEngagementRepositionRequested(Attacker);

		if (bHasExistingEngagement && !bRepositionRequested)
		{
			const FVector CandidateLocation = GetCandidateWorldLocation(ExistingAssignment->CandidateIndex);
			const float TargetDistanceSquared = FVector::DistSquared2D(TargetActor->GetActorLocation(), CandidateLocation);

			if (TargetDistanceSquared <= FMath::Square(Attacker->GetMaxEngagementDistance())
				&& CanOccupyCandidate(Attacker, ExistingAssignment->CandidateIndex, PendingAssignments))
			{
				CandidateIndex = ExistingAssignment->CandidateIndex;
			}
		}

		if (CandidateIndex == INDEX_NONE && bHasExistingEngagement && bRepositionRequested)
		{
			CandidateIndex = FindBestEngagementCandidate(
				Attacker, PendingAssignments, true, ExistingAssignment->CandidateIndex);
		}

		if (CandidateIndex == INDEX_NONE && bHasExistingEngagement)
		{
			const FVector CandidateLocation = GetCandidateWorldLocation(ExistingAssignment->CandidateIndex);
			const float TargetDistanceSquared = FVector::DistSquared2D(TargetActor->GetActorLocation(), CandidateLocation);

			if (TargetDistanceSquared <= FMath::Square(Attacker->GetMaxEngagementDistance())
				&& CanOccupyCandidate(Attacker, ExistingAssignment->CandidateIndex, PendingAssignments))
			{
				CandidateIndex = ExistingAssignment->CandidateIndex;
			}
		}

		if (CandidateIndex == INDEX_NONE)
		{
			CandidateIndex = FindBestEngagementCandidate(Attacker, PendingAssignments);
		}

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
	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		if (PendingAssignments.Contains(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* ExistingAssignment = Assignments.Find(Attacker);

		if (!ExistingAssignment || ExistingAssignment->State != EARPGEngagementState::Pressure
			|| IsPressureRetargetDue(Attacker, CurrentTime)
			|| !Candidates.IsValidIndex(ExistingAssignment->CandidateIndex))
		{
			continue;
		}

		const FVector CandidateLocation = GetCandidateWorldLocation(ExistingAssignment->CandidateIndex);

		if (FVector::DistSquared2D(TargetActor->GetActorLocation(), CandidateLocation) < FMath::Square(PressureMinDistance))
		{
			continue;
		}

		if (!CanOccupyCandidate(Attacker, ExistingAssignment->CandidateIndex, PendingAssignments))
		{
			continue;
		}

		PendingAssignments.Add(Attacker, *ExistingAssignment);
	}

	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		if (PendingAssignments.Contains(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* ExistingAssignment = Assignments.Find(Attacker);
		const bool bWasPressure = ExistingAssignment && ExistingAssignment->State == EARPGEngagementState::Pressure;
		const bool bForceRetarget = bWasPressure && IsPressureRetargetDue(Attacker, CurrentTime);

		const int32 CandidateIndex = FindBestPressureCandidate(Attacker, PendingAssignments, bForceRetarget);

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
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : CoordinatedAttackers)
	{
		UARPGCombatantComponent* Attacker = WeakAttacker.Get();

		if (!IsValid(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* PreviousAssignment = Assignments.Find(WeakAttacker);
		const FARPGCombatAssignment* NewAssignment = PendingAssignments.Find(WeakAttacker);

		if (!NewAssignment || !Candidates.IsValidIndex(NewAssignment->CandidateIndex))
		{
			NextPressureRetargetTimes.Remove(Attacker);
			RemainingAttacksBeforeReposition.Remove(Attacker);

			Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);
			continue;
		}

		const bool bWasEngaged = PreviousAssignment && PreviousAssignment->State == EARPGEngagementState::Engaged;
		const bool bWasPressure = PreviousAssignment && PreviousAssignment->State == EARPGEngagementState::Pressure;
		const bool bRepositionRequested = EngagementRepositionRequests.Contains(WeakAttacker);
		const bool bPressureRetargetWasDue = bWasPressure && IsPressureRetargetDue(Attacker, CurrentTime);

		if (NewAssignment->State == EARPGEngagementState::Engaged)
		{
			NextPressureRetargetTimes.Remove(Attacker);

			if (!bWasEngaged || bRepositionRequested)
			{
				ResetEngagementAttackCount(Attacker);
			}
		}
		else
		{
			RemainingAttacksBeforeReposition.Remove(Attacker);

			if (NewAssignment->State == EARPGEngagementState::Pressure)
			{
				if (!bWasPressure || bPressureRetargetWasDue)
				{
					ScheduleNextPressureRetarget(Attacker, CurrentTime);
				}
			}
			else
			{
				NextPressureRetargetTimes.Remove(Attacker);
			}
		}

		Attacker->SetCoordination(NewAssignment->State, GetCandidateWorldLocation(NewAssignment->CandidateIndex));
	}

	Assignments = MoveTemp(PendingAssignments);
	EngagementRepositionRequests.Reset();
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
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments,
	const bool bForceReposition, const int32 ExistingCandidateIndex) const
{
	if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()) || !IsValid(TargetActor))
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	FVector ExistingLocation = FVector::ZeroVector;
	const bool bHasExistingLocation = bForceReposition && Candidates.IsValidIndex(ExistingCandidateIndex);

	if (bHasExistingLocation)
	{
		ExistingLocation = GetCandidateWorldLocation(ExistingCandidateIndex);
	}

	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = FLT_MAX;

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FVector CandidateLocation = GetCandidateWorldLocation(Index);

		if (FVector::DistSquared2D(TargetLocation, CandidateLocation) > FMath::Square(Attacker->GetMaxEngagementDistance()))
		{
			continue;
		}

		if (bHasExistingLocation
			&& FVector::DistSquared2D(ExistingLocation, CandidateLocation) < FMath::Square(EngagementRepositionMinMoveDistance))
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
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments,
	const bool bForceRetarget) const
{
	if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()) || !IsValid(TargetActor))
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	const FARPGCombatAssignment* ExistingAssignment = Assignments.Find(Attacker);

	FVector ExistingLocation = FVector::ZeroVector;
	bool bHasExistingPressureLocation = false;

	if (ExistingAssignment && ExistingAssignment->State == EARPGEngagementState::Pressure
		&& Candidates.IsValidIndex(ExistingAssignment->CandidateIndex))
	{
		ExistingLocation = GetCandidateWorldLocation(ExistingAssignment->CandidateIndex);
		bHasExistingPressureLocation = true;
	}

	auto FindCandidate = [&](const bool bRequireMovement)
	{
		int32 BestIndex = INDEX_NONE;
		float BestValue = FLT_MAX;

		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			const FVector CandidateLocation = GetCandidateWorldLocation(Index);
			const float TargetDistance = FVector::Dist2D(TargetLocation, CandidateLocation);

			if (TargetDistance < PressureMinDistance)
			{
				continue;
			}

			if (bRequireMovement && bHasExistingPressureLocation
				&& FVector::DistSquared2D(ExistingLocation, CandidateLocation) < FMath::Square(PressureRetargetMinMoveDistance))
			{
				continue;
			}

			if (!CanOccupyCandidate(Attacker, Index, PendingAssignments))
			{
				continue;
			}

			const float TravelDistance = FVector::Dist2D(AttackerLocation, CandidateLocation);
			const float Value = TargetDistance * PressureRadialWeight + TravelDistance;

			if (Value >= BestValue)
			{
				continue;
			}

			BestValue = Value;
			BestIndex = Index;
		}

		return BestIndex;
	};

	if (bForceRetarget)
	{
		const int32 RetargetIndex = FindCandidate(true);

		if (RetargetIndex != INDEX_NONE)
		{
			return RetargetIndex;
		}
	}

	return FindCandidate(false);
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

bool UARPGCombatCoordinationComponent::IsPressureRetargetDue(const UARPGCombatantComponent* Attacker,
	const double CurrentTime) const
{
	if (!IsValid(Attacker))
	{
		return false;
	}

	const double* NextRetargetTime = NextPressureRetargetTimes.Find(Attacker);

	return !NextRetargetTime || CurrentTime >= *NextRetargetTime;
}

void UARPGCombatCoordinationComponent::ScheduleNextPressureRetarget(UARPGCombatantComponent* Attacker,
	const double CurrentTime)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	const float MaxInterval = FMath::Max(PressureRetargetMinInterval, PressureRetargetMaxInterval);
	const float Delay = FMath::FRandRange(PressureRetargetMinInterval, MaxInterval);

	NextPressureRetargetTimes.FindOrAdd(Attacker) = CurrentTime + Delay;
}

void UARPGCombatCoordinationComponent::HandleAttackerAttackCompleted(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker) || EngagementRepositionRequests.Contains(Attacker))
	{
		return;
	}

	const FARPGCombatAssignment* Assignment = Assignments.Find(Attacker);

	if (!Assignment || Assignment->State != EARPGEngagementState::Engaged)
	{
		return;
	}

	int32* RemainingAttacks = RemainingAttacksBeforeReposition.Find(Attacker);

	if (!RemainingAttacks)
	{
		ResetEngagementAttackCount(Attacker);
		RemainingAttacks = RemainingAttacksBeforeReposition.Find(Attacker);
	}

	if (!RemainingAttacks)
	{
		return;
	}

	--(*RemainingAttacks);

	if (*RemainingAttacks > 0)
	{
		return;
	}

	EngagementRepositionRequests.Add(Attacker);
	bAssignmentsDirty = true;
}

void UARPGCombatCoordinationComponent::ResetEngagementAttackCount(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	const int32 MinAttacks = FMath::Max(1, Attacker->GetMinAttacksBeforeReposition());
	const int32 MaxAttacks = FMath::Max(MinAttacks, Attacker->GetMaxAttacksBeforeReposition());

	RemainingAttacksBeforeReposition.FindOrAdd(Attacker) = FMath::RandRange(MinAttacks, MaxAttacks);
}

bool UARPGCombatCoordinationComponent::IsEngagementRepositionRequested(const UARPGCombatantComponent* Attacker) const
{
	if (!IsValid(Attacker))
	{
		return false;
	}

	return EngagementRepositionRequests.Contains(Attacker);
}

void UARPGCombatCoordinationComponent::BindAttackerEvents(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker) || AttackCompletedDelegateHandles.Contains(Attacker))
	{
		return;
	}

	const FDelegateHandle Handle = Attacker->OnAttackCompleted.AddUObject(
		this, &UARPGCombatCoordinationComponent::HandleAttackerAttackCompleted);

	AttackCompletedDelegateHandles.Add(Attacker, Handle);
}

void UARPGCombatCoordinationComponent::UnbindAttackerEvents(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	const FDelegateHandle* Handle = AttackCompletedDelegateHandles.Find(Attacker);

	if (!Handle)
	{
		return;
	}

	Attacker->OnAttackCompleted.Remove(*Handle);
	AttackCompletedDelegateHandles.Remove(Attacker);
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
