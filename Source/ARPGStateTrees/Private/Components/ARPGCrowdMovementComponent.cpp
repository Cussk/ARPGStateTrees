// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/ARPGCrowdMovementComponent.h"

#include "GameFramework/Character.h"

UARPGCrowdMovementComponent::UARPGCrowdMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGCrowdMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
}

void UARPGCrowdMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPassThroughActors();

	Character = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UARPGCrowdMovementComponent::SetRightOfWayRequested(const bool bRequested)
{
	bRightOfWayRequested = bRequested;
}

bool UARPGCrowdMovementComponent::IsRightOfWayRequested() const
{
	return bRightOfWayRequested;
}

void UARPGCrowdMovementComponent::SetPassThroughActors(const TSet<AActor*>& DesiredActors)
{
	if (!IsValid(Character))
	{
		return;
	}

	for (auto Iterator = PassThroughActors.CreateIterator(); Iterator; ++Iterator)
	{
		AActor* Actor = Iterator->Get();

		if (IsValid(Actor) && DesiredActors.Contains(Actor))
		{
			continue;
		}

		if (IsValid(Actor))
		{
			Character->MoveIgnoreActorRemove(Actor);
		}

		Iterator.RemoveCurrent();
	}

	for (AActor* Actor : DesiredActors)
	{
		if (!IsValid(Actor) || Actor == Character)
		{
			continue;
		}

		const TWeakObjectPtr<AActor> WeakActor = Actor;

		if (PassThroughActors.Contains(WeakActor))
		{
			continue;
		}

		Character->MoveIgnoreActorAdd(Actor);
		PassThroughActors.Add(WeakActor);
	}
}

void UARPGCrowdMovementComponent::RemovePassThroughActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> WeakActor = Actor;

	if (PassThroughActors.Remove(WeakActor) == 0)
	{
		return;
	}

	if (IsValid(Character))
	{
		Character->MoveIgnoreActorRemove(Actor);
	}
}

void UARPGCrowdMovementComponent::ClearPassThroughActors()
{
	if (IsValid(Character))
	{
		for (const TWeakObjectPtr<AActor>& WeakActor : PassThroughActors)
		{
			if (AActor* Actor = WeakActor.Get())
			{
				Character->MoveIgnoreActorRemove(Actor);
			}
		}
	}

	PassThroughActors.Reset();
	bRightOfWayRequested = false;
}