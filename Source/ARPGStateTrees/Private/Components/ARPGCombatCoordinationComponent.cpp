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
			Attacker->SetCoordination(EARPGCoordinationState::None, FVector::ZeroVector);
		}
	}

	AttackCompletedDelegateHandles.Reset();
	SupportAbilityUsedDelegateHandles.Reset();

	Attackers.Reset();
	CoordinatedAttackers.Reset();
	MeleeAssignments.Reset();
	RangedAssignments.Reset();
	SupportAssignments.Reset();

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

	bMeleeAssignmentsDirty = true;
	bRangedAssignmentsDirty = true;
	bSupportAssignmentsDirty = true;

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
	MeleeAssignments.Remove(Attacker);
	RangedAssignments.Remove(Attacker);
	NextPressureRetargetTimes.Remove(Attacker);
	RemainingAttacksBeforeReposition.Remove(Attacker);
	EngagementRepositionRequests.Remove(Attacker);
	RangedRepositionRequests.Remove(Attacker);
	RangedRepositionRequestTimes.Remove(Attacker);
	RemainingRangedAttacksBeforeReposition.Remove(Attacker);
	SupportAssignments.Remove(Attacker);
	SupportRepositionRequests.Remove(Attacker);
	SupportRepositionRequestTimes.Remove(Attacker);
	NextSupportAbilityTimes.Remove(Attacker);

	Attacker->SetTargetInAttackRange(false);
	Attacker->SetCoordination(EARPGCoordinationState::None, FVector::ZeroVector);
	Attacker->SetSupportOpportunity(false);

	bMeleeAssignmentsDirty = true;
	bRangedAssignmentsDirty = true;
	bSupportAssignmentsDirty = true;

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

	if (UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(this))
	{
		if (ActiveMeleeQueryId != INDEX_NONE)
		{
			QueryManager->AbortQuery(ActiveMeleeQueryId);
		}

		if (ActiveRangedQueryId != INDEX_NONE)
		{
			QueryManager->AbortQuery(ActiveRangedQueryId);
		}
		
		if (ActiveSupportQueryId != INDEX_NONE)
		{
			QueryManager->AbortQuery(ActiveSupportQueryId);
		}
	}
	
	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : Attackers)
	{
		if (UARPGCombatantComponent* Attacker = WeakAttacker.Get())
		{
			Attacker->SetSupportOpportunity(false);
		}
	}
	
	NextSupportAbilityTimes.Reset();

	ActiveMeleeQueryId = INDEX_NONE;
	ActiveRangedQueryId = INDEX_NONE;
	ActiveSupportQueryId = INDEX_NONE;

	MeleeCandidates.Reset();
	RangedCandidates.Reset();
	MeleeAssignments.Reset();
	RangedAssignments.Reset();
	CoordinatedAttackers.Reset();
	NextPressureRetargetTimes.Reset();
	RemainingAttacksBeforeReposition.Reset();
	EngagementRepositionRequests.Reset();
	RangedRepositionRequests.Reset();
	RangedRepositionRequestTimes.Reset();
	RemainingRangedAttacksBeforeReposition.Reset();
	SupportCandidates.Reset();
	SupportAssignments.Reset();
	SupportRepositionRequests.Reset();
	SupportRepositionRequestTimes.Reset();

	LastAssignmentReevaluationTime = 0.0;
	
	RangedFieldOrigin = FVector::ZeroVector;
	RangedPendingQueryOrigin = FVector::ZeroVector;
	SupportFieldOrigin = FVector::ZeroVector;
	SupportPendingQueryOrigin = FVector::ZeroVector;

	bMeleeFieldInitialized = false;
	bRangedFieldInitialized = false;
	bMeleeAssignmentsDirty = false;
	bRangedAssignmentsDirty = false;
	bSupportFieldInitialized = false;
	bSupportAssignmentsDirty = false;
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
		bMeleeAssignmentsDirty = true;
		bRangedAssignmentsDirty = true;
		bSupportAssignmentsDirty = true;
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

	const FVector TargetLocation = TargetActor->GetActorLocation();

	const bool bHasMeleeAttackers = HasCoordinatedAttackers(EARPGPositioningMode::Melee);
	const bool bHasRangedAttackers = HasCoordinatedAttackers(EARPGPositioningMode::Ranged);
	const bool bHasSupportAttackers = HasCoordinatedAttackers(EARPGPositioningMode::Support);

	if (bHasMeleeAttackers)
	{
		if (!bMeleeFieldInitialized)
		{
			RequestMeleeFieldRefresh();
		}
		else
		{
			UpdateMeleeAssignmentGoals();

			if (FVector::DistSquared2D(TargetLocation, MeleeFieldOrigin) >= FMath::Square(FieldRefreshDistance))
			{
				RequestMeleeFieldRefresh();
			}
		}
	}

	if (bHasRangedAttackers)
	{
		if (!bRangedFieldInitialized)
		{
			RequestRangedFieldRefresh();
		}
		else
		{
			if (FVector::DistSquared2D(TargetLocation, RangedFieldOrigin) >= FMath::Square(RangedFieldRefreshDistance))
			{
				RequestRangedFieldRefresh();
			}

			if (UpdateRangedRepositionRequests(World->GetTimeSeconds()))
			{
				bRangedAssignmentsDirty = true;
			}
		}
	}
	
	if (bHasSupportAttackers)
	{
		if (!bSupportFieldInitialized)
		{
			RequestSupportFieldRefresh();
		}
		else if (FVector::DistSquared2D(TargetLocation, SupportFieldOrigin) >= FMath::Square(SupportFieldRefreshDistance))
		{
			RequestSupportFieldRefresh();
		}
	}

	const double CurrentTime = World->GetTimeSeconds();

	if (CurrentTime - LastAssignmentReevaluationTime >= AssignmentReevaluationInterval)
	{
		if (bHasMeleeAttackers)
		{
			bMeleeAssignmentsDirty = true;
		}

		if (bHasRangedAttackers)
		{
			bRangedAssignmentsDirty = true;
		}

		LastAssignmentReevaluationTime = CurrentTime;
	}
	
	if (bHasSupportAttackers && UpdateSupportRepositionRequests(CurrentTime))
	{
		bSupportAssignmentsDirty = true;
	}

	if (bMeleeAssignmentsDirty || bRangedAssignmentsDirty || bSupportAssignmentsDirty)
	{
		RebuildAssignments();
	}
	
	if (bHasSupportAttackers)
	{
		UpdateSupportOpportunities(CurrentTime);
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
			MeleeAssignments.Remove(*Iterator);
			RangedAssignments.Remove(*Iterator);
			NextPressureRetargetTimes.Remove(*Iterator);
			RemainingAttacksBeforeReposition.Remove(*Iterator);
			EngagementRepositionRequests.Remove(*Iterator);
			Iterator.RemoveCurrent();
			bChanged = true;
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
		TargetLocation, Attacker->GetCombatantActor()->GetActorLocation());

		const float AttackRange = Attacker->GetBasicAttackRange();
		const bool bInAttackRange = DistanceSquared <= FMath::Square(AttackRange);

		Attacker->SetTargetInAttackRange(bInAttackRange);

		float ActivationRadius = CoordinationActivationRadius;
		float DeactivationRadius = CoordinationDeactivationRadius;

		switch (Attacker->GetPositioningMode())
		{
		case EARPGPositioningMode::Ranged:
			ActivationRadius = RangedCoordinationActivationRadius;
			DeactivationRadius = RangedCoordinationDeactivationRadius;
			break;

		case EARPGPositioningMode::Support:
			ActivationRadius = SupportCoordinationActivationRadius;
			DeactivationRadius = SupportCoordinationDeactivationRadius;
			break;

		default:
			break;
		}

		const bool bIsCoordinated = CoordinatedAttackers.Contains(Attacker);

		if (bIsCoordinated)
		{
			if (DistanceSquared > FMath::Square(DeactivationRadius))
			{
				CoordinatedAttackers.Remove(Attacker);
				MeleeAssignments.Remove(Attacker);
				RangedAssignments.Remove(Attacker);
				NextPressureRetargetTimes.Remove(Attacker);
				RemainingAttacksBeforeReposition.Remove(Attacker);
				EngagementRepositionRequests.Remove(Attacker);
				SupportAssignments.Remove(Attacker);
				SupportRepositionRequests.Remove(Attacker);
				SupportRepositionRequestTimes.Remove(Attacker);

				Attacker->SetCoordination(EARPGCoordinationState::None, FVector::ZeroVector);
				Attacker->SetSupportOpportunity(false);
				bChanged = true;
			}

			continue;
		}

		if (DistanceSquared <= FMath::Square(ActivationRadius))
		{
			CoordinatedAttackers.Add(Attacker);
			bChanged = true;
		}
	}

	return bChanged;
}

bool UARPGCombatCoordinationComponent::HasCoordinatedAttackers(const EARPGPositioningMode PositioningMode) const
{
	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : CoordinatedAttackers)
	{
		const UARPGCombatantComponent* Attacker = WeakAttacker.Get();

		if (IsValid(Attacker) && Attacker->GetPositioningMode() == PositioningMode)
		{
			return true;
		}
	}

	return false;
}

void UARPGCombatCoordinationComponent::RequestMeleeFieldRefresh()
{
	if (!IsValid(MeleeEngagementQuery) || !IsValid(TargetActor) || ActiveMeleeQueryId != INDEX_NONE)
	{
		return;
	}

	MeleePendingQueryOrigin = TargetActor->GetActorLocation();

	FEnvQueryRequest QueryRequest(MeleeEngagementQuery, TargetActor);
	ActiveMeleeQueryId = QueryRequest.Execute(EEnvQueryRunMode::AllMatching, this,
		&UARPGCombatCoordinationComponent::OnMeleeFieldQueryFinished);
}

void UARPGCombatCoordinationComponent::OnMeleeFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	ActiveMeleeQueryId = INDEX_NONE;

	if (!Result.IsValid() || !Result->IsSuccessful() || !IsValid(TargetActor) || Attackers.IsEmpty())
	{
		return;
	}

	MeleeCandidates.Reset();
	MeleeCandidates.Reserve(Result->Items.Num());

	for (int32 Index = 0; Index < Result->Items.Num(); ++Index)
	{
		FARPGCoordinationCandidate& Candidate = MeleeCandidates.AddDefaulted_GetRef();
		Candidate.Location = Result->GetItemAsLocation(Index);
	}

	MeleeFieldOrigin = MeleePendingQueryOrigin;
	bMeleeFieldInitialized = true;
	bMeleeAssignmentsDirty = true;

	RebuildAssignments();
}

void UARPGCombatCoordinationComponent::RequestRangedFieldRefresh()
{
	if (!IsValid(RangedCoordinationQuery) || !IsValid(TargetActor) || ActiveRangedQueryId != INDEX_NONE)
	{
		return;
	}

	RangedPendingQueryOrigin = TargetActor->GetActorLocation();

	FEnvQueryRequest QueryRequest(RangedCoordinationQuery, TargetActor);
	ActiveRangedQueryId = QueryRequest.Execute(EEnvQueryRunMode::AllMatching, this,
		&UARPGCombatCoordinationComponent::OnRangedFieldQueryFinished);
}

void UARPGCombatCoordinationComponent::OnRangedFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	ActiveRangedQueryId = INDEX_NONE;

	if (!Result.IsValid() || !Result->IsSuccessful() || !IsValid(TargetActor) || Attackers.IsEmpty())
	{
		return;
	}

	RangedCandidates.Reset();
	RangedCandidates.Reserve(Result->Items.Num());

	for (int32 Index = 0; Index < Result->Items.Num(); ++Index)
	{
		FARPGCoordinationCandidate& Candidate = RangedCandidates.AddDefaulted_GetRef();
		Candidate.Location = Result->GetItemAsLocation(Index);
	}

	RangedFieldOrigin = RangedPendingQueryOrigin;
	bRangedFieldInitialized = true;
	bRangedAssignmentsDirty = true;

	RebuildAssignments();
}

void UARPGCombatCoordinationComponent::RequestSupportFieldRefresh()
{
	if (!IsValid(SupportCoordinationQuery) || !IsValid(TargetActor) || ActiveSupportQueryId != INDEX_NONE)
	{
		return;
	}

	SupportPendingQueryOrigin = TargetActor->GetActorLocation();

	FEnvQueryRequest QueryRequest(SupportCoordinationQuery, TargetActor);
	ActiveSupportQueryId = QueryRequest.Execute(EEnvQueryRunMode::AllMatching, this,
		&UARPGCombatCoordinationComponent::OnSupportFieldQueryFinished);
}

void UARPGCombatCoordinationComponent::OnSupportFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	ActiveSupportQueryId = INDEX_NONE;

	if (!Result.IsValid() || !Result->IsSuccessful() || !IsValid(TargetActor) || Attackers.IsEmpty())
	{
		return;
	}

	SupportCandidates.Reset();
	SupportCandidates.Reserve(Result->Items.Num());

	for (int32 Index = 0; Index < Result->Items.Num(); ++Index)
	{
		FARPGCoordinationCandidate& Candidate = SupportCandidates.AddDefaulted_GetRef();
		Candidate.Location = Result->GetItemAsLocation(Index);
	}

	SupportFieldOrigin = SupportPendingQueryOrigin;
	bSupportFieldInitialized = true;
	bSupportAssignmentsDirty = true;

	RebuildAssignments();
}

void UARPGCombatCoordinationComponent::RebuildAssignments()
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	TArray<UARPGCombatantComponent*> MeleeAttackers;
	TArray<UARPGCombatantComponent*> RangedAttackers;
	TArray<UARPGCombatantComponent*> SupportAttackers;

	MeleeAttackers.Reserve(CoordinatedAttackers.Num());
	RangedAttackers.Reserve(CoordinatedAttackers.Num());
	SupportAttackers.Reserve(CoordinatedAttackers.Num());

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : CoordinatedAttackers)
	{
		UARPGCombatantComponent* Attacker = WeakAttacker.Get();

		if (!IsValid(Attacker))
		{
			continue;
		}

		switch (Attacker->GetPositioningMode())
		{
		case EARPGPositioningMode::Ranged:
			RangedAttackers.Add(Attacker);
			break;

		case EARPGPositioningMode::Support:
			SupportAttackers.Add(Attacker);
			break;

		default:
			MeleeAttackers.Add(Attacker);
			break;
		}
	}

	if (MeleeAttackers.IsEmpty())
	{
		MeleeAssignments.Reset();
		bMeleeAssignmentsDirty = false;
	}
	else if (bMeleeAssignmentsDirty && bMeleeFieldInitialized
		&& FVector::DistSquared2D(TargetLocation, MeleeFieldOrigin) < FMath::Square(FieldRefreshDistance))
	{
		MeleeAttackers.Sort([this, TargetLocation](const UARPGCombatantComponent& A, const UARPGCombatantComponent& B)
		{
			if (A.GetCoordinationPriority() != B.GetCoordinationPriority())
			{
				return A.GetCoordinationPriority() > B.GetCoordinationPriority();
			}

			const bool bAEngaged = A.GetCoordinationState() == EARPGCoordinationState::MeleeEngaged;
			const bool bBEngaged = B.GetCoordinationState() == EARPGCoordinationState::MeleeEngaged;

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

		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> PendingMeleeAssignments;

		if (!MeleeCandidates.IsEmpty())
		{
			BuildEngagementAssignments(MeleeAttackers, PendingMeleeAssignments);
			BuildPressureAssignments(MeleeAttackers, PendingMeleeAssignments);
		}

		CommitMeleeAssignments(MeleeAttackers, PendingMeleeAssignments);

		LastMeleeGoalUpdateOrigin = TargetLocation;
		bMeleeAssignmentsDirty = false;
	}

	if (RangedAttackers.IsEmpty())
	{
		RangedAssignments.Reset();
		bRangedAssignmentsDirty = false;
	}
	else if (bRangedAssignmentsDirty && bRangedFieldInitialized
		&& FVector::DistSquared2D(TargetLocation, RangedFieldOrigin) < FMath::Square(RangedFieldRefreshDistance))
	{
		RangedAttackers.Sort([TargetLocation](const UARPGCombatantComponent& A, const UARPGCombatantComponent& B)
		{
			if (A.GetCoordinationPriority() != B.GetCoordinationPriority())
			{
				return A.GetCoordinationPriority() > B.GetCoordinationPriority();
			}

			const bool bAHasAssignment = A.GetCoordinationState() == EARPGCoordinationState::Ranged;
			const bool bBHasAssignment = B.GetCoordinationState() == EARPGCoordinationState::Ranged;

			if (bAHasAssignment != bBHasAssignment)
			{
				return bAHasAssignment;
			}

			const float DistanceA = FVector::DistSquared2D(A.GetCombatantActor()->GetActorLocation(), TargetLocation);
			const float DistanceB = FVector::DistSquared2D(B.GetCombatantActor()->GetActorLocation(), TargetLocation);

			return DistanceA < DistanceB;
		});

		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> PendingRangedAssignments;

		if (!RangedCandidates.IsEmpty())
		{
			BuildRangedAssignments(RangedAttackers, PendingRangedAssignments);
		}

		CommitRangedAssignments(RangedAttackers, PendingRangedAssignments);

		LastRangedGoalUpdateOrigin = TargetLocation;
		bRangedAssignmentsDirty = false;
	}
	
	if (SupportAttackers.IsEmpty())
	{
		SupportAssignments.Reset();
		SupportRepositionRequests.Reset();
		SupportRepositionRequestTimes.Reset();
		bSupportAssignmentsDirty = false;
	}
	else if (bSupportAssignmentsDirty && bSupportFieldInitialized
		&& FVector::DistSquared2D(TargetLocation, SupportFieldOrigin) < FMath::Square(SupportFieldRefreshDistance))
	{
		SupportAttackers.Sort([TargetLocation](const UARPGCombatantComponent& A, const UARPGCombatantComponent& B)
		{
			if (A.GetCoordinationPriority() != B.GetCoordinationPriority())
			{
				return A.GetCoordinationPriority() > B.GetCoordinationPriority();
			}

			const bool bAHasAssignment = A.GetCoordinationState() == EARPGCoordinationState::Support;
			const bool bBHasAssignment = B.GetCoordinationState() == EARPGCoordinationState::Support;

			if (bAHasAssignment != bBHasAssignment)
			{
				return bAHasAssignment;
			}

			const float DistanceA = FVector::DistSquared2D(A.GetCombatantActor()->GetActorLocation(), TargetLocation);
			const float DistanceB = FVector::DistSquared2D(B.GetCombatantActor()->GetActorLocation(), TargetLocation);

			return DistanceA < DistanceB;
		});

		TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment> PendingSupportAssignments;

		if (!SupportCandidates.IsEmpty())
		{
			BuildSupportAssignments(SupportAttackers, PendingSupportAssignments);
		}

		CommitSupportAssignments(SupportAttackers, PendingSupportAssignments);

		bSupportAssignmentsDirty = false;
	}
}

void UARPGCombatCoordinationComponent::BuildEngagementAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		int32 CandidateIndex = INDEX_NONE;

		const FARPGCombatAssignment* ExistingAssignment = MeleeAssignments.Find(Attacker);
		const bool bHasExistingEngagement = ExistingAssignment
			&& ExistingAssignment->State == EARPGCoordinationState::MeleeEngaged
			&& MeleeCandidates.IsValidIndex(ExistingAssignment->CandidateIndex);

		const bool bRepositionRequested = IsEngagementRepositionRequested(Attacker);

		if (bHasExistingEngagement && !bRepositionRequested)
		{
			const FVector CandidateLocation = GetMeleeCandidateWorldLocation(ExistingAssignment->CandidateIndex);
			const float TargetDistanceSquared = FVector::DistSquared2D(TargetActor->GetActorLocation(), CandidateLocation);

			if (TargetDistanceSquared <= FMath::Square(Attacker->GetMaxEngagementDistance())
				&& CanOccupyMeleeCandidate(Attacker, ExistingAssignment->CandidateIndex, PendingAssignments))
			{
				CandidateIndex = ExistingAssignment->CandidateIndex;
			}
		}

		if (CandidateIndex == INDEX_NONE && bHasExistingEngagement && bRepositionRequested)
		{
			CandidateIndex = FindBestEngagementCandidate(Attacker, PendingAssignments, true, ExistingAssignment->CandidateIndex);
		}

		if (CandidateIndex == INDEX_NONE && bHasExistingEngagement)
		{
			const FVector CandidateLocation = GetMeleeCandidateWorldLocation(ExistingAssignment->CandidateIndex);
			const float TargetDistanceSquared = FVector::DistSquared2D(TargetActor->GetActorLocation(), CandidateLocation);

			if (TargetDistanceSquared <= FMath::Square(Attacker->GetMaxEngagementDistance())
				&& CanOccupyMeleeCandidate(Attacker, ExistingAssignment->CandidateIndex, PendingAssignments))
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
		Assignment.State = EARPGCoordinationState::MeleeEngaged;
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

		const FARPGCombatAssignment* ExistingAssignment = MeleeAssignments.Find(Attacker);

		if (!ExistingAssignment || ExistingAssignment->State != EARPGCoordinationState::MeleePressure
			|| IsPressureRetargetDue(Attacker, CurrentTime)
			|| !MeleeCandidates.IsValidIndex(ExistingAssignment->CandidateIndex))
		{
			continue;
		}

		const FVector CandidateLocation = GetMeleeCandidateWorldLocation(ExistingAssignment->CandidateIndex);

		if (FVector::DistSquared2D(TargetActor->GetActorLocation(), CandidateLocation) < FMath::Square(PressureMinDistance))
		{
			continue;
		}

		if (!CanOccupyMeleeCandidate(Attacker, ExistingAssignment->CandidateIndex, PendingAssignments))
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

		const FARPGCombatAssignment* ExistingAssignment = MeleeAssignments.Find(Attacker);
		const bool bWasPressure = ExistingAssignment && ExistingAssignment->State == EARPGCoordinationState::MeleePressure;
		const bool bForceRetarget = bWasPressure && IsPressureRetargetDue(Attacker, CurrentTime);

		const int32 CandidateIndex = FindBestPressureCandidate(Attacker, PendingAssignments, bForceRetarget);

		if (CandidateIndex == INDEX_NONE)
		{
			continue;
		}

		FARPGCombatAssignment Assignment;
		Assignment.State = EARPGCoordinationState::MeleePressure;
		Assignment.CandidateIndex = CandidateIndex;

		PendingAssignments.Add(Attacker, Assignment);
	}
}

void UARPGCombatCoordinationComponent::BuildRangedAssignments(const TArray<UARPGCombatantComponent*>& SortedAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	const FVector TargetLocation = TargetActor->GetActorLocation();

	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		const FARPGCombatAssignment* ExistingAssignment = RangedAssignments.Find(Attacker);

		if (!ExistingAssignment || ExistingAssignment->State != EARPGCoordinationState::Ranged
			|| IsRangedRepositionRequested(Attacker))
		{
			continue;
		}

		const float TargetDistance = FVector::Dist2D(TargetLocation, ExistingAssignment->Location);

		if (TargetDistance < RangedHoldMinDistance || TargetDistance > RangedHoldMaxDistance)
		{
			continue;
		}

		if (!CanOccupyRangedLocation(Attacker, ExistingAssignment->Location, PendingAssignments))
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

		const FARPGCombatAssignment* ExistingAssignment = RangedAssignments.Find(Attacker);
		const bool bRepositionRequested = IsRangedRepositionRequested(Attacker);

		const FVector ExistingLocation = ExistingAssignment
			? ExistingAssignment->Location
			: FVector::ZeroVector;

		const int32 CandidateIndex = FindBestRangedCandidate(
			Attacker, PendingAssignments, bRepositionRequested, ExistingLocation);

		if (CandidateIndex == INDEX_NONE)
		{
			if (ExistingAssignment && CanOccupyRangedLocation(Attacker, ExistingAssignment->Location, PendingAssignments))
			{
				PendingAssignments.Add(Attacker, *ExistingAssignment);
			}

			continue;
		}

		FARPGCombatAssignment Assignment;
		Assignment.State = EARPGCoordinationState::Ranged;
		Assignment.CandidateIndex = CandidateIndex;
		Assignment.Location = GetRangedCandidateWorldLocation(CandidateIndex);

		PendingAssignments.Add(Attacker, Assignment);
	}
}

void UARPGCombatCoordinationComponent::BuildSupportAssignments(const TArray<UARPGCombatantComponent*>& SupportAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	for (UARPGCombatantComponent* Attacker : SupportAttackers)
	{
		const FARPGCombatAssignment* ExistingAssignment = SupportAssignments.Find(Attacker);

		if (!ExistingAssignment || ExistingAssignment->State != EARPGCoordinationState::Support
			|| SupportRepositionRequests.Contains(Attacker))
		{
			continue;
		}

		if (!CanOccupySupportLocation(Attacker, ExistingAssignment->Location, PendingAssignments))
		{
			continue;
		}

		PendingAssignments.Add(Attacker, *ExistingAssignment);
	}

	for (UARPGCombatantComponent* Attacker : SupportAttackers)
	{
		if (PendingAssignments.Contains(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* ExistingAssignment = SupportAssignments.Find(Attacker);
		const int32 CandidateIndex = FindBestSupportCandidate(Attacker, PendingAssignments);

		if (CandidateIndex == INDEX_NONE)
		{
			if (ExistingAssignment
				&& ExistingAssignment->State == EARPGCoordinationState::Support
				&& CanOccupySupportLocation(Attacker, ExistingAssignment->Location, PendingAssignments))
			{
				PendingAssignments.Add(Attacker, *ExistingAssignment);
			}

			continue;
		}

		FARPGCombatAssignment Assignment;
		Assignment.State = EARPGCoordinationState::Support;
		Assignment.CandidateIndex = CandidateIndex;
		Assignment.Location = GetSupportCandidateWorldLocation(CandidateIndex);

		PendingAssignments.Add(Attacker, Assignment);
	}
}

void UARPGCombatCoordinationComponent::CommitMeleeAssignments(const TArray<UARPGCombatantComponent*>& MeleeAttackers,
                                                              TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	for (UARPGCombatantComponent* Attacker : MeleeAttackers)
	{
		if (!IsValid(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* PreviousAssignment = MeleeAssignments.Find(Attacker);
		const FARPGCombatAssignment* NewAssignment = PendingAssignments.Find(Attacker);

		if (!NewAssignment || !MeleeCandidates.IsValidIndex(NewAssignment->CandidateIndex))
		{
			NextPressureRetargetTimes.Remove(Attacker);
			RemainingAttacksBeforeReposition.Remove(Attacker);

			Attacker->SetCoordination(EARPGCoordinationState::None, FVector::ZeroVector);
			continue;
		}

		const bool bWasEngaged = PreviousAssignment && PreviousAssignment->State == EARPGCoordinationState::MeleeEngaged;
		const bool bWasPressure = PreviousAssignment && PreviousAssignment->State == EARPGCoordinationState::MeleePressure;
		const bool bRepositionRequested = EngagementRepositionRequests.Contains(Attacker);
		const bool bPressureRetargetWasDue = bWasPressure && IsPressureRetargetDue(Attacker, CurrentTime);

		if (NewAssignment->State == EARPGCoordinationState::MeleeEngaged)
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

			if (NewAssignment->State == EARPGCoordinationState::MeleePressure)
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

		Attacker->SetCoordination(NewAssignment->State, GetMeleeCandidateWorldLocation(NewAssignment->CandidateIndex));
	}

	MeleeAssignments = MoveTemp(PendingAssignments);
	EngagementRepositionRequests.Reset();
}

void UARPGCombatCoordinationComponent::CommitRangedAssignments(const TArray<UARPGCombatantComponent*>& RangedAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments)
{
	for (UARPGCombatantComponent* Attacker : RangedAttackers)
	{
		if (!IsValid(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* PreviousAssignment = RangedAssignments.Find(Attacker);
		const FARPGCombatAssignment* NewAssignment = PendingAssignments.Find(Attacker);

		if (!NewAssignment)
		{
			RemainingRangedAttacksBeforeReposition.Remove(Attacker);
			RangedRepositionRequestTimes.Remove(Attacker);

			Attacker->SetCoordination(EARPGCoordinationState::None, FVector::ZeroVector);
			continue;
		}

		const bool bWasRanged = PreviousAssignment && PreviousAssignment->State == EARPGCoordinationState::Ranged;
		const bool bRepositionRequested = RangedRepositionRequests.Contains(Attacker);

		if (!bWasRanged || bRepositionRequested)
		{
			ResetRangedAttackCount(Attacker);
		}

		RangedRepositionRequestTimes.Remove(Attacker);

		Attacker->SetCoordination(EARPGCoordinationState::Ranged, NewAssignment->Location);
	}

	RangedAssignments = MoveTemp(PendingAssignments);
	RangedRepositionRequests.Reset();
}

void UARPGCombatCoordinationComponent::CommitSupportAssignments(const TArray<UARPGCombatantComponent*>& SupportAttackers,
	TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments)
{
	for (UARPGCombatantComponent* Attacker : SupportAttackers)
	{
		if (!IsValid(Attacker))
		{
			continue;
		}

		const FARPGCombatAssignment* NewAssignment = PendingAssignments.Find(Attacker);

		if (!NewAssignment)
		{
			SupportRepositionRequestTimes.Remove(Attacker);
			NextSupportAbilityTimes.Remove(Attacker);

			Attacker->SetSupportOpportunity(false);
			Attacker->SetCoordination(EARPGCoordinationState::None, FVector::ZeroVector);
			continue;
		}

		SupportRepositionRequestTimes.Remove(Attacker);
		Attacker->SetCoordination(EARPGCoordinationState::Support, NewAssignment->Location);
	}

	SupportAssignments = MoveTemp(PendingAssignments);
	SupportRepositionRequests.Reset();
}

void UARPGCombatCoordinationComponent::UpdateMeleeAssignmentGoals()
{
	if (!IsValid(TargetActor) || MeleeAssignments.IsEmpty())
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	if (FVector::DistSquared2D(TargetLocation, LastMeleeGoalUpdateOrigin) < FMath::Square(AssignmentGoalUpdateDistance))
	{
		return;
	}

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : MeleeAssignments)
	{
		UARPGCombatantComponent* Attacker = Pair.Key.Get();

		if (!IsValid(Attacker) || !MeleeCandidates.IsValidIndex(Pair.Value.CandidateIndex))
		{
			continue;
		}

		Attacker->SetCoordination(Pair.Value.State, GetMeleeCandidateWorldLocation(Pair.Value.CandidateIndex));
	}

	LastMeleeGoalUpdateOrigin = TargetLocation;
}

bool UARPGCombatCoordinationComponent::UpdateRangedRepositionRequests(const double CurrentTime)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	bool bChanged = false;
	const FVector TargetLocation = TargetActor->GetActorLocation();

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : RangedAssignments)
	{
		UARPGCombatantComponent* Attacker = Pair.Key.Get();

		if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()))
		{
			continue;
		}

		if (RangedRepositionRequests.Contains(Attacker))
		{
			continue;
		}

		const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
		const float DistanceToAssignment = FVector::Dist2D(AttackerLocation, Pair.Value.Location);

		if (DistanceToAssignment > RangedAssignmentSettledDistance)
		{
			RangedRepositionRequestTimes.Remove(Attacker);
			continue;
		}

		const float TargetDistance = FVector::Dist2D(TargetLocation, AttackerLocation);
		const bool bPositionValid = TargetDistance >= RangedHoldMinDistance && TargetDistance <= RangedHoldMaxDistance;

		if (bPositionValid)
		{
			RangedRepositionRequestTimes.Remove(Attacker);
			continue;
		}

		double* RepositionTime = RangedRepositionRequestTimes.Find(Attacker);

		if (!RepositionTime)
		{
			const float MaxDelay = FMath::Max(RangedReactionMinDelay, RangedReactionMaxDelay);
			const float Delay = FMath::FRandRange(RangedReactionMinDelay, MaxDelay);

			RangedRepositionRequestTimes.Add(Attacker, CurrentTime + Delay);
			continue;
		}

		if (CurrentTime < *RepositionTime)
		{
			continue;
		}

		RangedRepositionRequestTimes.Remove(Attacker);
		RangedRepositionRequests.Add(Attacker);
		bChanged = true;
	}

	return bChanged;
}

bool UARPGCombatCoordinationComponent::IsRangedRepositionRequested(const UARPGCombatantComponent* Attacker) const
{
	return IsValid(Attacker) && RangedRepositionRequests.Contains(Attacker);
}

void UARPGCombatCoordinationComponent::ResetRangedAttackCount(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	const int32 MinAttacks = FMath::Max(1, Attacker->GetMinAttacksBeforeReposition());
	const int32 MaxAttacks = FMath::Max(MinAttacks, Attacker->GetMaxAttacksBeforeReposition());

	RemainingRangedAttacksBeforeReposition.FindOrAdd(Attacker) = FMath::RandRange(MinAttacks, MaxAttacks);
}

bool UARPGCombatCoordinationComponent::UpdateSupportRepositionRequests(const double CurrentTime)
{
	if (!IsValid(TargetActor) || SupportAssignments.IsEmpty())
	{
		return false;
	}

	bool bChanged = false;
	const FVector TargetLocation = TargetActor->GetActorLocation();

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : SupportAssignments)
	{
		UARPGCombatantComponent* Attacker = Pair.Key.Get();

		if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()))
		{
			continue;
		}

		if (SupportRepositionRequests.Contains(Attacker))
		{
			continue;
		}

		const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
		const float DistanceToAssignment = FVector::Dist2D(AttackerLocation, Pair.Value.Location);

		if (DistanceToAssignment > SupportAssignmentSettledDistance)
		{
			SupportRepositionRequestTimes.Remove(Attacker);
			continue;
		}

		const float TargetDistance = FVector::Dist2D(TargetLocation, AttackerLocation);

		if (TargetDistance >= SupportHoldMinDistance && TargetDistance <= SupportHoldMaxDistance)
		{
			SupportRepositionRequestTimes.Remove(Attacker);
			continue;
		}

		double* RepositionTime = SupportRepositionRequestTimes.Find(Attacker);

		if (!RepositionTime)
		{
			const float MaxDelay = FMath::Max(SupportReactionMinDelay, SupportReactionMaxDelay);
			const float Delay = FMath::FRandRange(SupportReactionMinDelay, MaxDelay);

			SupportRepositionRequestTimes.Add(Attacker, CurrentTime + Delay);
			continue;
		}

		if (CurrentTime < *RepositionTime)
		{
			continue;
		}

		SupportRepositionRequestTimes.Remove(Attacker);
		SupportRepositionRequests.Add(Attacker);
		bChanged = true;
	}

	return bChanged;
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
	const bool bHasExistingLocation = bForceReposition && MeleeCandidates.IsValidIndex(ExistingCandidateIndex);

	if (bHasExistingLocation)
	{
		ExistingLocation = GetMeleeCandidateWorldLocation(ExistingCandidateIndex);
	}

	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = FLT_MAX;

	for (int32 Index = 0; Index < MeleeCandidates.Num(); ++Index)
	{
		const FVector CandidateLocation = GetMeleeCandidateWorldLocation(Index);

		if (FVector::DistSquared2D(TargetLocation, CandidateLocation) > FMath::Square(Attacker->GetMaxEngagementDistance()))
		{
			continue;
		}

		if (bHasExistingLocation
			&& FVector::DistSquared2D(ExistingLocation, CandidateLocation) < FMath::Square(EngagementRepositionMinMoveDistance))
		{
			continue;
		}

		if (!CanOccupyMeleeCandidate(Attacker, Index, PendingAssignments))
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

	const FARPGCombatAssignment* ExistingAssignment = MeleeAssignments.Find(Attacker);

	FVector ExistingLocation = FVector::ZeroVector;
	bool bHasExistingPressureLocation = false;

	if (ExistingAssignment && ExistingAssignment->State == EARPGCoordinationState::MeleePressure
		&& MeleeCandidates.IsValidIndex(ExistingAssignment->CandidateIndex))
	{
		ExistingLocation = GetMeleeCandidateWorldLocation(ExistingAssignment->CandidateIndex);
		bHasExistingPressureLocation = true;
	}

	auto FindCandidate = [&](const bool bRequireMovement)
	{
		int32 BestIndex = INDEX_NONE;
		float BestValue = FLT_MAX;

		for (int32 Index = 0; Index < MeleeCandidates.Num(); ++Index)
		{
			const FVector CandidateLocation = GetMeleeCandidateWorldLocation(Index);
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

			if (!CanOccupyMeleeCandidate(Attacker, Index, PendingAssignments))
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

int32 UARPGCombatCoordinationComponent::FindBestRangedCandidate(const UARPGCombatantComponent* Attacker,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments,
	const bool bForceReposition, const FVector& ExistingLocation) const
{
	if (!IsValid(Attacker) || !IsValid(Attacker->GetCombatantActor()) || !IsValid(TargetActor))
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = FLT_MAX;

	for (int32 Index = 0; Index < RangedCandidates.Num(); ++Index)
	{
		const FVector CandidateLocation = GetRangedCandidateWorldLocation(Index);
		const float TargetDistance = FVector::Dist2D(TargetLocation, CandidateLocation);

		if (TargetDistance < RangedMinDistance || TargetDistance > RangedMaxDistance)
		{
			continue;
		}

		if (bForceReposition)
		{
			if (FVector::DistSquared2D(AttackerLocation, CandidateLocation)
				< FMath::Square(RangedRepositionMinMoveDistance))
			{
				continue;
			}

			if (!ExistingLocation.IsNearlyZero()
				&& FVector::DistSquared2D(ExistingLocation, CandidateLocation)
					< FMath::Square(RangedRepositionMinMoveDistance))
			{
				continue;
			}
		}

		if (!CanOccupyRangedLocation(Attacker, CandidateLocation, PendingAssignments))
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

int32 UARPGCombatCoordinationComponent::FindBestSupportCandidate(const UARPGCombatantComponent* Attacker,
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

	for (int32 Index = 0; Index < SupportCandidates.Num(); ++Index)
	{
		const FVector CandidateLocation = GetSupportCandidateWorldLocation(Index);
		const float TargetDistance = FVector::Dist2D(TargetLocation, CandidateLocation);

		if (TargetDistance < SupportMinDistance || TargetDistance > SupportMaxDistance)
		{
			continue;
		}

		if (!CanOccupySupportLocation(Attacker, CandidateLocation, PendingAssignments))
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

bool UARPGCombatCoordinationComponent::CanOccupyMeleeCandidate(const UARPGCombatantComponent* Attacker, const int32 CandidateIndex,
                                                               const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	if (!IsValid(Attacker) || !MeleeCandidates.IsValidIndex(CandidateIndex))
	{
		return false;
	}

	const FVector CandidateLocation = GetMeleeCandidateWorldLocation(CandidateIndex);

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : PendingAssignments)
	{
		const UARPGCombatantComponent* Other = Pair.Key.Get();

		if (!IsValid(Other) || !MeleeCandidates.IsValidIndex(Pair.Value.CandidateIndex))
		{
			continue;
		}

		const float RequiredSeparation = Attacker->GetOccupancyRadius() + Other->GetOccupancyRadius() + AssignmentSeparation;
		const FVector OtherLocation = GetMeleeCandidateWorldLocation(Pair.Value.CandidateIndex);

		if (FVector::DistSquared2D(CandidateLocation, OtherLocation) < FMath::Square(RequiredSeparation))
		{
			return false;
		}
	}

	return true;
}

bool UARPGCombatCoordinationComponent::CanOccupyRangedLocation(const UARPGCombatantComponent* Attacker,
	const FVector& Location,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	if (!IsValid(Attacker))
	{
		return false;
	}

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : PendingAssignments)
	{
		const UARPGCombatantComponent* Other = Pair.Key.Get();

		if (!IsValid(Other))
		{
			continue;
		}

		const float RequiredSeparation = Attacker->GetOccupancyRadius()
			+ Other->GetOccupancyRadius()
			+ AssignmentSeparation;

		if (FVector::DistSquared2D(Location, Pair.Value.Location) < FMath::Square(RequiredSeparation))
		{
			return false;
		}
	}

	return true;
}

bool UARPGCombatCoordinationComponent::CanOccupySupportLocation(const UARPGCombatantComponent* Attacker,
	const FVector& Location,
	const TMap<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& PendingAssignments) const
{
	if (!IsValid(Attacker))
	{
		return false;
	}

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : PendingAssignments)
	{
		const UARPGCombatantComponent* Other = Pair.Key.Get();

		if (!IsValid(Other))
		{
			continue;
		}

		const float RequiredSeparation = Attacker->GetOccupancyRadius()
			+ Other->GetOccupancyRadius()
			+ AssignmentSeparation;

		if (FVector::DistSquared2D(Location, Pair.Value.Location) < FMath::Square(RequiredSeparation))
		{
			return false;
		}
	}

	return true;
}

bool UARPGCombatCoordinationComponent::IsPressureRetargetDue(const UARPGCombatantComponent* Attacker, const double CurrentTime) const
{
	if (!IsValid(Attacker))
	{
		return false;
	}

	const double* NextRetargetTime = NextPressureRetargetTimes.Find(Attacker);

	return !NextRetargetTime || CurrentTime >= *NextRetargetTime;
}

void UARPGCombatCoordinationComponent::ScheduleNextPressureRetarget(UARPGCombatantComponent* Attacker, const double CurrentTime)
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
	if (!IsValid(Attacker))
	{
		return;
	}

	const FARPGCombatAssignment* MeleeAssignment = MeleeAssignments.Find(Attacker);

	if (MeleeAssignment && MeleeAssignment->State == EARPGCoordinationState::MeleeEngaged)
	{
		if (EngagementRepositionRequests.Contains(Attacker))
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

		if (--(*RemainingAttacks) <= 0)
		{
			EngagementRepositionRequests.Add(Attacker);
			bMeleeAssignmentsDirty = true;
		}

		return;
	}

	const FARPGCombatAssignment* RangedAssignment = RangedAssignments.Find(Attacker);

	if (!RangedAssignment || RangedAssignment->State != EARPGCoordinationState::Ranged
		|| RangedRepositionRequests.Contains(Attacker))
	{
		return;
	}
	
	if (!IsValid(Attacker->GetCombatantActor()))
	{
		return;
	}

	const float DistanceToAssignmentSquared = FVector::DistSquared2D(
		Attacker->GetCombatantActor()->GetActorLocation(),
		RangedAssignment->Location);

	if (DistanceToAssignmentSquared > FMath::Square(RangedAssignmentSettledDistance))
	{
		return;
	}

	int32* RemainingAttacks = RemainingRangedAttacksBeforeReposition.Find(Attacker);

	if (!RemainingAttacks)
	{
		ResetRangedAttackCount(Attacker);
		RemainingAttacks = RemainingRangedAttacksBeforeReposition.Find(Attacker);
	}

	if (!RemainingAttacks)
	{
		return;
	}

	if (--(*RemainingAttacks) <= 0)
	{
		RangedRepositionRequests.Add(Attacker);
		bRangedAssignmentsDirty = true;
	}
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

void UARPGCombatCoordinationComponent::UpdateSupportOpportunities(const double CurrentTime)
{
	if (SupportAssignments.IsEmpty())
	{
		return;
	}

	const int32 AllyCount = GetCoordinatedNonSupportAllyCount();

	for (const TPair<TWeakObjectPtr<UARPGCombatantComponent>, FARPGCombatAssignment>& Pair : SupportAssignments)
	{
		UARPGCombatantComponent* Support = Pair.Key.Get();

		if (!IsValid(Support) || !IsValid(Support->GetCombatantActor()))
		{
			continue;
		}

		if (Pair.Value.State != EARPGCoordinationState::Support)
		{
			continue;
		}

		if (AllyCount < MinimumSupportAllies)
		{
			Support->SetSupportOpportunity(false);
			NextSupportAbilityTimes.Remove(Support);
			continue;
		}

		if (Support->HasSupportOpportunity())
		{
			continue;
		}

		double* NextAbilityTime = NextSupportAbilityTimes.Find(Support);

		if (!NextAbilityTime)
		{
			ScheduleSupportAbility(
				Support,
				CurrentTime,
				SupportInitialAbilityDelayMin,
				SupportInitialAbilityDelayMax);

			continue;
		}

		const float DistanceToAssignment = FVector::Dist2D(
			Support->GetCombatantActor()->GetActorLocation(),
			Pair.Value.Location);

		if (DistanceToAssignment > SupportAssignmentSettledDistance)
		{
			continue;
		}

		if (CurrentTime < *NextAbilityTime)
		{
			continue;
		}

		NextSupportAbilityTimes.Remove(Support);
		Support->SetSupportOpportunity(true);
	}
}

void UARPGCombatCoordinationComponent::ScheduleSupportAbility(UARPGCombatantComponent* Attacker,
                                                              const double CurrentTime, const float MinDelay, const float MaxDelay)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	const float SafeMaxDelay = FMath::Max(MinDelay, MaxDelay);
	const float Delay = FMath::FRandRange(MinDelay, SafeMaxDelay);

	NextSupportAbilityTimes.FindOrAdd(Attacker) = CurrentTime + Delay;
}

void UARPGCombatCoordinationComponent::HandleSupportAbilityUsed(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker) || Attacker->GetPositioningMode() != EARPGPositioningMode::Support)
	{
		return;
	}

	Attacker->SetSupportOpportunity(false);

	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	ScheduleSupportAbility(
		Attacker,
		World->GetTimeSeconds(),
		SupportAbilityCooldownMin,
		SupportAbilityCooldownMax);
}

void UARPGCombatCoordinationComponent::BindAttackerEvents(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	if (!AttackCompletedDelegateHandles.Contains(Attacker))
	{
		const FDelegateHandle Handle = Attacker->OnAttackCompleted.AddUObject(
			this, &UARPGCombatCoordinationComponent::HandleAttackerAttackCompleted);

		AttackCompletedDelegateHandles.Add(Attacker, Handle);
	}

	if (!SupportAbilityUsedDelegateHandles.Contains(Attacker))
	{
		const FDelegateHandle Handle = Attacker->OnSupportAbilityUsed.AddUObject(
			this, &UARPGCombatCoordinationComponent::HandleSupportAbilityUsed);

		SupportAbilityUsedDelegateHandles.Add(Attacker, Handle);
	}
}

void UARPGCombatCoordinationComponent::UnbindAttackerEvents(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker))
	{
		return;
	}

	if (const FDelegateHandle* Handle = AttackCompletedDelegateHandles.Find(Attacker))
	{
		Attacker->OnAttackCompleted.Remove(*Handle);
		AttackCompletedDelegateHandles.Remove(Attacker);
	}

	if (const FDelegateHandle* Handle = SupportAbilityUsedDelegateHandles.Find(Attacker))
	{
		Attacker->OnSupportAbilityUsed.Remove(*Handle);
		SupportAbilityUsedDelegateHandles.Remove(Attacker);
	}
}

int32 UARPGCombatCoordinationComponent::GetCoordinatedNonSupportAllyCount() const
{
	int32 Count = 0;

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : CoordinatedAttackers)
	{
		const UARPGCombatantComponent* Attacker = WeakAttacker.Get();

		if (!IsValid(Attacker) || Attacker->GetPositioningMode() == EARPGPositioningMode::Support)
		{
			continue;
		}

		++Count;
	}

	return Count;
}

FVector UARPGCombatCoordinationComponent::GetMeleeCandidateWorldLocation(const int32 CandidateIndex) const
{
	if (!MeleeCandidates.IsValidIndex(CandidateIndex) || !IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	FVector TargetDelta = TargetActor->GetActorLocation() - MeleeFieldOrigin;
	TargetDelta.Z = 0.0f;

	return MeleeCandidates[CandidateIndex].Location + TargetDelta;
}

FVector UARPGCombatCoordinationComponent::GetRangedCandidateWorldLocation(const int32 CandidateIndex) const
{
	if (!RangedCandidates.IsValidIndex(CandidateIndex))
	{
		return FVector::ZeroVector;
	}

	return RangedCandidates[CandidateIndex].Location;
}

FVector UARPGCombatCoordinationComponent::GetSupportCandidateWorldLocation(const int32 CandidateIndex) const
{
	if (!SupportCandidates.IsValidIndex(CandidateIndex))
	{
		return FVector::ZeroVector;
	}

	return SupportCandidates[CandidateIndex].Location;
}
