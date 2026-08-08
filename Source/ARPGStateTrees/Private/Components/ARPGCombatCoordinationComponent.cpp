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
			Attacker->SetAttackPermission(false);
		}
	}

	Attackers.Reset();

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

	if (bFieldInitialized)
	{
		RebuildAssignments();
	}
	else
	{
		RequestFieldRefresh();
	}
}

void UARPGCombatCoordinationComponent::UnregisterAttacker(UARPGCombatantComponent* Attacker)
{
	if (!IsValid(Attacker) || Attackers.Remove(Attacker) == 0)
	{
		return;
	}

	Attacker->SetCoordination(EARPGEngagementState::None, FVector::ZeroVector);
	Attacker->SetAttackPermission(false);

	bAssignmentsDirty = true;

	if (Attackers.IsEmpty())
	{
		StopCoordination();
		return;
	}

	RebuildAssignments();
}

void UARPGCombatCoordinationComponent::StartCoordination()
{
	UpdateCoordination();
	GetWorld()->GetTimerManager().SetTimer(CoordinationTimer, this, &UARPGCombatCoordinationComponent::UpdateCoordination, CoordinationInterval, true);
}

void UARPGCombatCoordinationComponent::StopCoordination()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CoordinationTimer);
	}

	Candidates.Reset();

	ActiveQueryId = INDEX_NONE;
	bFieldInitialized = false;
	bAssignmentsDirty = false;
}

void UARPGCombatCoordinationComponent::UpdateCoordination()
{
	if (!TargetActor || Attackers.IsEmpty())
	{
		return;
	}

	for (auto Iterator = Attackers.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator->IsValid())
		{
			Iterator.RemoveCurrent();
			bAssignmentsDirty = true;
		}
	}

	if (Attackers.IsEmpty())
	{
		StopCoordination();
		return;
	}

	const bool bTargetMoved = FVector::DistSquared2D(TargetActor->GetActorLocation(), LastQueryOrigin) >= FMath::Square(FieldRefreshDistance);

	if (!bFieldInitialized || bTargetMoved)
	{
		RequestFieldRefresh();
		return;
	}

	if (bAssignmentsDirty)
	{
		RebuildAssignments();
		return;
	}

	TArray<UARPGCombatantComponent*> ValidAttackers;
	ValidAttackers.Reserve(Attackers.Num());

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : Attackers)
	{
		if (UARPGCombatantComponent* Attacker = WeakAttacker.Get())
		{
			ValidAttackers.Add(Attacker);
		}
	}

	UpdateAttackPermissions(ValidAttackers);
}

void UARPGCombatCoordinationComponent::RequestFieldRefresh()
{
	if (!IsValid(EngagementQuery) || !IsValid(TargetActor) || ActiveQueryId != INDEX_NONE)
	{
		return;
	}

	FEnvQueryRequest QueryRequest(EngagementQuery, TargetActor);
	ActiveQueryId = QueryRequest.Execute(EEnvQueryRunMode::AllMatching, this, &UARPGCombatCoordinationComponent::OnFieldQueryFinished);
}

void UARPGCombatCoordinationComponent::OnFieldQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	ActiveQueryId = INDEX_NONE;

	if (!Result.IsValid() || !Result->IsSuccessful() || !TargetActor)
	{
		return;
	}

	Candidates.Reset();
	Candidates.Reserve(Result->Items.Num());

	for (int32 Index = 0; Index < Result->Items.Num(); ++Index)
	{
		FARPGCoordinationCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Location = Result->GetItemAsLocation(Index);
		Candidate.Score = Result->GetItemScore(Index);
	}

	LastQueryOrigin = TargetActor->GetActorLocation();
	bFieldInitialized = true;
	bAssignmentsDirty = true;

	RebuildAssignments();
}

void UARPGCombatCoordinationComponent::RebuildAssignments()
{
	if (!IsValid(TargetActor) || Candidates.IsEmpty())
	{
		return;
	}

	TArray<UARPGCombatantComponent*> SortedAttackers;

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakAttacker : Attackers)
	{
		if (UARPGCombatantComponent* Attacker = WeakAttacker.Get())
		{
			SortedAttackers.Add(Attacker);
			Attacker->SetAttackPermission(false);
		}
	}

	SortedAttackers.Sort([](const UARPGCombatantComponent& A, const UARPGCombatantComponent& B)
	{
		if (A.GetEngagementPriority() != B.GetEngagementPriority())
		{
			return A.GetEngagementPriority() > B.GetEngagementPriority();
		}

		return FVector::DistSquared2D(A.GetCombatantActor()->GetActorLocation(), A.GetCurrentTarget()->GetCombatantActor()->GetActorLocation())
			< FVector::DistSquared2D(B.GetCombatantActor()->GetActorLocation(), B.GetCurrentTarget()->GetCombatantActor()->GetActorLocation());
	});

	TSet<int32> OccupiedCandidates;

	BuildEngagementAssignments(SortedAttackers, OccupiedCandidates);
	BuildPressureAssignments(SortedAttackers, OccupiedCandidates);
	UpdateAttackPermissions(SortedAttackers);

	bAssignmentsDirty = false;
}

void UARPGCombatCoordinationComponent::BuildEngagementAssignments(TArray<UARPGCombatantComponent*>& SortedAttackers, TSet<int32>& OccupiedCandidates)
{
	for (UARPGCombatantComponent* Attacker : SortedAttackers)
	{
		const int32 CandidateIndex = FindBestEngagementCandidate(Attacker, OccupiedCandidates);

		if (CandidateIndex == INDEX_NONE)
		{
			continue;
		}

		OccupiedCandidates.Add(CandidateIndex);
		Attacker->SetCoordination(EARPGEngagementState::Engaged, Candidates[CandidateIndex].Location);
	}
}

void UARPGCombatCoordinationComponent::BuildPressureAssignments(const TArray<UARPGCombatantComponent*>& AttackersToAssign, const TSet<int32>& OccupiedCandidates)
{
	TSet<int32> PressureCandidates = OccupiedCandidates;

	for (UARPGCombatantComponent* Attacker : AttackersToAssign)
	{
		if (Attacker->GetEngagementState() == EARPGEngagementState::Engaged)
		{
			continue;
		}

		const int32 CandidateIndex = FindBestPressureCandidate(Attacker, PressureCandidates);

		if (CandidateIndex == INDEX_NONE)
		{
			Attacker->SetCoordination(EARPGEngagementState::Pressure, TargetActor->GetActorLocation());
			continue;
		}

		PressureCandidates.Add(CandidateIndex);
		Attacker->SetCoordination(EARPGEngagementState::Pressure, Candidates[CandidateIndex].Location);
	}
}

bool UARPGCombatCoordinationComponent::CanOccupyCandidate(const UARPGCombatantComponent* Attacker, const int32 CandidateIndex) const
{
	if (!IsValid(Attacker) || !Candidates.IsValidIndex(CandidateIndex))
	{
		return false;
	}

	const float RequiredRadius = Attacker->GetOccupancyRadius() + AssignmentSeparation;

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakOther : Attackers)
	{
		const UARPGCombatantComponent* Other = WeakOther.Get();

		if (!Other || Other == Attacker || Other->GetEngagementState() != EARPGEngagementState::Engaged)
		{
			continue;
		}

		const float RequiredSeparation = RequiredRadius + Other->GetOccupancyRadius();

		if (FVector::DistSquared2D(Candidates[CandidateIndex].Location, Other->GetEngagementLocation()) < FMath::Square(RequiredSeparation))
		{
			return false;
		}
	}

	return true;
}

void UARPGCombatCoordinationComponent::UpdateAttackPermissions(const TArray<UARPGCombatantComponent*>& AttackersToUpdate)
{
	TArray<UARPGCombatantComponent*> EngagedAttackers;

	for (UARPGCombatantComponent* Attacker : AttackersToUpdate)
	{
		Attacker->SetAttackPermission(false);

		if (Attacker->GetEngagementState() == EARPGEngagementState::Engaged)
		{
			EngagedAttackers.Add(Attacker);
		}
	}

	if (EngagedAttackers.IsEmpty())
	{
		AttackSelectionOffset = 0;
		return;
	}

	AttackSelectionOffset %= EngagedAttackers.Num();

	const int32 AttackCount = FMath::Min(MaxConcurrentAttackers, EngagedAttackers.Num());

	for (int32 Index = 0; Index < AttackCount; ++Index)
	{
		const int32 AttackerIndex = (AttackSelectionOffset + Index) % EngagedAttackers.Num();
		EngagedAttackers[AttackerIndex]->SetAttackPermission(true);
	}

	AttackSelectionOffset = (AttackSelectionOffset + AttackCount) % EngagedAttackers.Num();
}

int32 UARPGCombatCoordinationComponent::FindBestEngagementCandidate(const UARPGCombatantComponent* Attacker, const TSet<int32>& OccupiedCandidates) const
{
	if (!IsValid(Attacker) || !Attacker->GetCombatantActor() || !IsValid(TargetActor))
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();

	int32 BestIndex = INDEX_NONE;
	float BestValue = -FLT_MAX;

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (OccupiedCandidates.Contains(Index) || !CanOccupyCandidate(Attacker, Index))
		{
			continue;
		}

		const float TargetDistance = FVector::Dist2D(TargetActor->GetActorLocation(), Candidates[Index].Location);

		if (TargetDistance > Attacker->GetMaxEngagementDistance())
		{
			continue;
		}

		const float TravelDistance = FVector::Dist2D(AttackerLocation, Candidates[Index].Location);
		const float Value = Candidates[Index].Score * 1000.0f - TravelDistance;

		if (Value > BestValue)
		{
			BestValue = Value;
			BestIndex = Index;
		}
	}

	return BestIndex;
}

int32 UARPGCombatCoordinationComponent::FindBestPressureCandidate(const UARPGCombatantComponent* Attacker, const TSet<int32>& OccupiedCandidates) const
{
	if (!IsValid(Attacker) || !Attacker->GetCombatantActor() || !TargetActor)
	{
		return INDEX_NONE;
	}

	const FVector AttackerLocation = Attacker->GetCombatantActor()->GetActorLocation();

	int32 BestIndex = INDEX_NONE;
	float BestValue = -FLT_MAX;

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (OccupiedCandidates.Contains(Index))
		{
			continue;
		}

		if (FVector::DistSquared2D(TargetActor->GetActorLocation(), Candidates[Index].Location) < FMath::Square(PressureMinDistance))
		{
			continue;
		}

		const float TravelDistance = FVector::Dist2D(AttackerLocation, Candidates[Index].Location);
		const float Value = Candidates[Index].Score * 500.0f - TravelDistance;

		if (Value > BestValue)
		{
			BestValue = Value;
			BestIndex = Index;
		}
	}

	return BestIndex;
}