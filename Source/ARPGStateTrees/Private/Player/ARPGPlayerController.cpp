// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Player/ARPGPlayerController.h"

#include "Character/ARPGPlayerCharacter.h"
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

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (PlayerMappingContext)
			{
				InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
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

	EnhancedInputComponent->BindAction(MoveToAction, ETriggerEvent::Started, this, &AARPGPlayerController::OnMoveStarted);
	EnhancedInputComponent->BindAction(MoveToAction, ETriggerEvent::Completed, this, &AARPGPlayerController::OnMoveEnded);
	EnhancedInputComponent->BindAction(MoveToAction, ETriggerEvent::Canceled, this, &AARPGPlayerController::OnMoveEnded);
}

void AARPGPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	NavigationComponent.Reset();

	if (const AARPGPlayerCharacter* ARPGCharacter = Cast<AARPGPlayerCharacter>(InPawn))
	{
		NavigationComponent = ARPGCharacter->GetARPGNavigationComponent();
	}
}

void AARPGPlayerController::OnMoveStarted(const FInputActionValue&)
{
	RequestCursorMove(false);

	GetWorldTimerManager().ClearTimer(HeldMoveTimerHandle);
	GetWorldTimerManager().SetTimer(HeldMoveTimerHandle, this, &AARPGPlayerController::UpdateHeldMove, HeldMoveUpdateInterval, true);
}

void AARPGPlayerController::OnMoveEnded(const FInputActionValue&)
{
	GetWorldTimerManager().ClearTimer(HeldMoveTimerHandle);
}

void AARPGPlayerController::UpdateHeldMove()
{
	RequestCursorMove(true);
}

void AARPGPlayerController::RequestCursorMove(const bool bContinuousUpdate) const
{
	UARPGNavigationComponent* CachedNavigationComponent = NavigationComponent.Get();

	if (!CachedNavigationComponent)
	{
		return;
	}

	FVector WorldLocation;

	if (!ResolveCursorWorldLocation(WorldLocation))
	{
		return;
	}

	if (bContinuousUpdate)
	{
		CachedNavigationComponent->UpdateMoveDestination(WorldLocation);
	}
	else
	{
		CachedNavigationComponent->RequestMoveToLocation(WorldLocation);
	}
}

bool AARPGPlayerController::ResolveCursorWorldLocation(FVector& OutWorldLocation) const
{
	FHitResult CursorHit;

	if (!GetHitResultUnderCursor(ECC_Visibility, false, CursorHit) || !CursorHit.bBlockingHit)
	{
		return false;
	}

	OutWorldLocation = CursorHit.ImpactPoint;
	return true;
}