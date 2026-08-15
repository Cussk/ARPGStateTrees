// Copyright Kyle Cuss and Cuss Programming 2026.


#include "AI/ARPGCompanionAIController.h"

#include "Character/ARPGCompanionCharacter.h"
#include "Components/StateTreeAIComponent.h"

AARPGCompanionAIController::AARPGCompanionAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	BrainComponent = StateTreeComponent;

	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
}

void AARPGCompanionAIController::OnPossess(APawn* InPawn)
{
	
	AARPGCompanionCharacter* CompanionCharacter = Cast<AARPGCompanionCharacter>(InPawn);
	if (IsValid(CompanionCharacter) && CompanionCharacter->GetStateTreeAsset())
	{
		StateTreeComponent->SetStateTree(CompanionCharacter->GetStateTreeAsset());
	}

	Super::OnPossess(InPawn);
}

