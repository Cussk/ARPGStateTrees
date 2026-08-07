// Copyright Kyle Cuss and Cuss Programming 2026.


#include "Player/ARPGPlayerController.h"

AARPGPlayerController::AARPGPlayerController()
{
}

void AARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameOnly());
}
