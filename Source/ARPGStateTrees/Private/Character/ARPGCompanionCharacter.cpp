// Copyright Kyle Cuss and Cuss Programming 2026.


#include "Character/ARPGCompanionCharacter.h"

#include "AI/ARPGCompanionAIController.h"
#include "Components/ARPGCombatantComponent.h"
#include "Components/ARPGCompanionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Types/ARPGCombatTypes.h"


AARPGCompanionCharacter::AARPGCompanionCharacter()
{
	CompanionComponent = CreateDefaultSubobject<UARPGCompanionComponent>(TEXT("CompanionComponent"));
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	AIControllerClass = AARPGCompanionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	bUseControllerRotationYaw = false;
	CombatantComponent->SetTeam(EARPGCombatTeam::Player);
}

UStateTree* AARPGCompanionCharacter::GetStateTreeAsset() const
{
	return StateTreeAsset;
}

UARPGCompanionComponent* AARPGCompanionCharacter::GetCompanionComponent() const
{
	return CompanionComponent;
}

