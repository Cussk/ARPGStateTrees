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
	CurrentTarget = NewTarget;

	OnTargetChanged.Broadcast(PreviousTarget, NewTarget);
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

AActor* UARPGCombatantComponent::GetCombatantActor() const
{
	return CombatantActor;
}

UARPGCombatantComponent* UARPGCombatantComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}