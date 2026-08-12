// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/ARPGCombatantComponent.h"

#include "Subsystems/ARPGCombatantRegistrySubsystem.h"

UARPGCombatantComponent::UARPGCombatantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGCombatantComponent::BeginPlay()
{
	Super::BeginPlay();

	CombatantActor = GetOwner();

	if (!IsValid(CombatantActor) || !CombatantActor->HasAuthority())
	{
		return;
	}

	if (UARPGCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UARPGCombatantRegistrySubsystem>())
	{
		Registry->RegisterCombatant(this);
	}
}

void UARPGCombatantComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(CombatantActor) && CombatantActor->HasAuthority())
	{
		if (UARPGCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UARPGCombatantRegistrySubsystem>())
		{
			Registry->UnregisterCombatant(this);
		}
	}

	CurrentTarget.Reset();
	CombatantActor = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UARPGCombatantComponent::SetTeam(const EARPGCombatTeam NewTeam)
{
	Team = NewTeam;
}

void UARPGCombatantComponent::SetTargetable(const bool bNewTargetable)
{
	bTargetable = bNewTargetable;
}

void UARPGCombatantComponent::SetCurrentTarget(UARPGCombatantComponent* NewTarget)
{
	if (CurrentTarget.Get() == NewTarget)
	{
		return;
	}

	UARPGCombatantComponent* PreviousTarget = CurrentTarget.Get();

	SetTargetInAttackRange(false);

	CurrentTarget = NewTarget;

	OnTargetChanged.Broadcast(PreviousTarget, NewTarget);
}

void UARPGCombatantComponent::SetCoordination(const EARPGCoordinationState NewState, const FVector& NewLocation)
{
	if (CoordinationState == NewState && CoordinationLocation.Equals(NewLocation, 1.0f))
	{
		return;
	}

	CoordinationState = NewState;
	CoordinationLocation = NewLocation;

	OnCoordinationChanged.Broadcast();
}

void UARPGCombatantComponent::SetTargetInAttackRange(const bool bInRange)
{
	if (bTargetInAttackRange == bInRange)
	{
		return;
	}

	bTargetInAttackRange = bInRange;
	OnAttackOpportunityChanged.Broadcast(bTargetInAttackRange);
}

float UARPGCombatantComponent::GetBasicAttackRange() const
{
	return BasicAttackRange;
}

bool UARPGCombatantComponent::IsTargetInAttackRange() const
{
	return bTargetInAttackRange;
}

EARPGCombatTeam UARPGCombatantComponent::GetTeam() const
{
	return Team;
}

bool UARPGCombatantComponent::IsTargetable() const
{
	return bTargetable && IsValid(CombatantActor);
}

bool UARPGCombatantComponent::IsHostileTo(const UARPGCombatantComponent* Other) const
{
	if (!Other || Team == EARPGCombatTeam::Neutral || Other->Team == EARPGCombatTeam::Neutral)
	{
		return false;
	}

	return Team != Other->Team;
}

EARPGCoordinationState UARPGCombatantComponent::GetCoordinationState() const
{
	return CoordinationState;
}

const FVector& UARPGCombatantComponent::GetCoordinationLocation() const
{
	return CoordinationLocation;
}

float UARPGCombatantComponent::GetOccupancyRadius() const
{
	return OccupancyRadius;
}

float UARPGCombatantComponent::GetMaxEngagementDistance() const
{
	return MaxEngagementDistance;
}

int32 UARPGCombatantComponent::GetCoordinationPriority() const
{
	return CoordinationPriority;
}

AActor* UARPGCombatantComponent::GetCombatantActor() const
{
	return CombatantActor;
}

UARPGCombatantComponent* UARPGCombatantComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}

EARPGPositioningMode UARPGCombatantComponent::GetPositioningMode() const
{
	return PositioningMode;
}

void UARPGCombatantComponent::NotifyAttackCompleted()
{
	OnAttackCompleted.Broadcast(this);
}

int32 UARPGCombatantComponent::GetMinAttacksBeforeReposition() const
{
	return MinAttacksBeforeReposition;
}

int32 UARPGCombatantComponent::GetMaxAttacksBeforeReposition() const
{
	return MaxAttacksBeforeReposition;
}
