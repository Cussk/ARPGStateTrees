// Copyright Kyle Cuss and Cuss Programming 2026.


#include "Character/ARPGCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/ARPGCombatantComponent.h"
#include "Components/ARPGNavigationComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AARPGCharacter::AARPGCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	
	NavigationComponent = CreateDefaultSubobject<UARPGNavigationComponent>(TEXT("NavigationComponent"));
	CombatantComponent = CreateDefaultSubobject<UARPGCombatantComponent>(TEXT("CombatantComponent"));
	
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

void AARPGCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

UARPGNavigationComponent* AARPGCharacter::GetARPGNavigationComponent() const
{
	return NavigationComponent;
}

UARPGCombatantComponent* AARPGCharacter::GetCombatantComponent() const
{
	return CombatantComponent;
}
