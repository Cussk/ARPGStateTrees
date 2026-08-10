// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Character/ARPGEnemyCharacter.h"

#include "AI/ARPGEnemyAIController.h"
#include "Components/ARPGCombatantComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Types/ARPGCombatTypes.h"

AARPGEnemyCharacter::AARPGEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AIControllerClass = AARPGEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	bUseControllerRotationYaw = false;
	CombatantComponent->SetTeam(EARPGCombatTeam::Enemy);
}

UStateTree* AARPGEnemyCharacter::GetStateTreeAsset() const
{
	return StateTreeAsset;
}
