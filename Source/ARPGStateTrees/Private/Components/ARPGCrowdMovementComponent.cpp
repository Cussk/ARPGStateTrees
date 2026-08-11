// ARPGCrowdMovementComponent.cpp

#include "Components/ARPGCrowdMovementComponent.h"

#include "Components/CapsuleComponent.h"
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
	SetRightOfWayRequested(false);

	Character = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UARPGCrowdMovementComponent::SetRightOfWayRequested(const bool bRequested)
{
	if (bRightOfWayRequested == bRequested)
	{
		return;
	}

	bRightOfWayRequested = bRequested;

	if (!IsValid(Character))
	{
		return;
	}

	Character->GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_Pawn, bRightOfWayRequested ? ECR_Ignore : ECR_Block);
}

bool UARPGCrowdMovementComponent::IsRightOfWayRequested() const
{
	return bRightOfWayRequested;
}