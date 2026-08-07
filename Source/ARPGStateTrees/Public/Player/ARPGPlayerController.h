// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ARPGPlayerController.generated.h"

UCLASS()
class ARPGSTATETREES_API AARPGPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AARPGPlayerController();
	
protected:
	virtual void BeginPlay() override;
};
