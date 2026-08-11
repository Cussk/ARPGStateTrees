// Copyright Kyle Cuss and Cuss Programming 2026.


#include "Character/ARPGPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/ARPGCombatantComponent.h"
#include "Components/ARPGCombatCoordinationComponent.h"
#include "Components/ARPGNavigationComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	
	NavigationComponent = CreateDefaultSubobject<UARPGNavigationComponent>(TEXT("NavigationComponent"));
	CombatCoordinationComponent = CreateDefaultSubobject<UARPGCombatCoordinationComponent>(TEXT("CombatCoordinationComponent"));
	
	SpringArmComponent->TargetArmLength = 900.0f;
	SpringArmComponent->SetUsingAbsoluteRotation(true);
	SpringArmComponent->SetRelativeRotation( FRotator( -55.0f, 0.0f, 0.0f ));
	SpringArmComponent->bDoCollisionTest = false;
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->bUsePawnControlRotation = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	
	CombatantComponent->SetTeam(EARPGCombatTeam::Player);
}

void AARPGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

UARPGNavigationComponent* AARPGPlayerCharacter::GetARPGNavigationComponent() const
{
	return NavigationComponent;
}

UARPGCombatCoordinationComponent* AARPGPlayerCharacter::GetCombatCoordinationComponent() const
{
	return CombatCoordinationComponent;
}
