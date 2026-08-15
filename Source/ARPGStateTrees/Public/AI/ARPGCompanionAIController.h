// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ARPGCompanionAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

UCLASS()
class ARPGSTATETREES_API AARPGCompanionAIController : public AAIController
{
	GENERATED_BODY()

public:
	AARPGCompanionAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};