// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/ARPGCompanionComponent.h"

#include "Components/ARPGCombatantComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UARPGCompanionComponent::UARPGCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UARPGCompanionComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshOwnerReferences();
}

void UARPGCompanionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UARPGCompanionComponent, CompanionOwnerActor);
}

void UARPGCompanionComponent::SetCompanionOwner(AActor* NewOwner)
{
	AActor* CompanionActor = GetOwner();

	if (!IsValid(CompanionActor) || !CompanionActor->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("ARPGCompanionComponent: SetCompanionOwner called without authority on %s."),
			*GetNameSafe(CompanionActor));
		return;
	}

	if (CompanionOwnerActor == NewOwner)
	{
		return;
	}

	CompanionOwnerActor = NewOwner;
	RefreshOwnerReferences();

	OnCompanionOwnerChanged.Broadcast(CompanionOwnerActor);
}

AActor* UARPGCompanionComponent::GetCompanionOwnerActor() const
{
	return CompanionOwnerActor;
}

UARPGCombatantComponent* UARPGCompanionComponent::GetCompanionOwnerCombatant() const
{
	return CompanionOwnerCombatant;
}

float UARPGCompanionComponent::GetFollowDistance() const
{
	return FollowDistance;
}

float UARPGCompanionComponent::GetCatchUpDistance() const
{
	return CatchUpDistance;
}

float UARPGCompanionComponent::GetMaximumLeashDistance() const
{
	return MaximumLeashDistance;
}

float UARPGCompanionComponent::GetFollowUpdateInterval() const
{
	return FollowUpdateInterval;
}

void UARPGCompanionComponent::OnRep_CompanionOwnerActor()
{
	RefreshOwnerReferences();

	OnCompanionOwnerChanged.Broadcast(CompanionOwnerActor);
}

void UARPGCompanionComponent::RefreshOwnerReferences()
{
	CompanionOwnerCombatant = IsValid(CompanionOwnerActor)
		? CompanionOwnerActor->FindComponentByClass<UARPGCombatantComponent>()
		: nullptr;
}
