// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Player/ARPGPlayerController.h"

#include "Character/ARPGCharacter.h"
#include "Components/ARPGNavigationComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

void AARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	SetInputMode(FInputModeGameOnly());

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (PlayerMappingContext)
			{
				InputSubsystem->AddMappingContext(PlayerMappingContext,0);
			}
		}
	}
}

void AARPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (!ensureMsgf(EnhancedInputComponent, TEXT("ARPGPlayerController requires Enhanced Input.")))
	{
		return;
	}

	if (!ensureMsgf(MoveToAction, TEXT("MoveToAction has not been configured.")))
	{
		return;
	}

	EnhancedInputComponent->BindAction(MoveToAction, ETriggerEvent::Started,	this, &AARPGPlayerController::OnMoveStarted);
	EnhancedInputComponent->BindAction(MoveToAction, ETriggerEvent::Triggered, this, &AARPGPlayerController::OnMoveTriggered);
}

void AARPGPlayerController::OnMoveStarted(const FInputActionValue&)
{
	RequestCursorMove(false);
}

void AARPGPlayerController::OnMoveTriggered(const FInputActionValue&)
{
	RequestCursorMove(true);
}

void AARPGPlayerController::RequestCursorMove(const bool bContinuousUpdate) const
{
	FVector WorldLocation;

	if (!ResolveCursorWorldLocation(WorldLocation))
	{
		return;
	}

	AARPGCharacter* ARPGCharacter =
		Cast<AARPGCharacter>(GetPawn());

	if (!ARPGCharacter)
	{
		return;
	}

	UARPGNavigationComponent* NavigationComponent =
		ARPGCharacter->GetARPGNavigationComponent();

	if (!NavigationComponent)
	{
		return;
	}

	if (bContinuousUpdate)
	{
		NavigationComponent->UpdateMoveDestination(WorldLocation);
	}
	else
	{
		NavigationComponent->RequestMoveToLocation(WorldLocation);
	}
}

bool AARPGPlayerController::ResolveCursorWorldLocation(FVector& OutWorldLocation) const
{
	FHitResult CursorHit;

	if (!GetHitResultUnderCursor(ECC_Visibility, false, CursorHit))
	{
		return false;
	}

	if (!CursorHit.bBlockingHit)
	{
		return false;
	}

	OutWorldLocation = CursorHit.ImpactPoint;

	return true;
}