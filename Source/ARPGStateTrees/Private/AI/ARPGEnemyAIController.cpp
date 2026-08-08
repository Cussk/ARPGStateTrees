// Copyright Kyle Cuss and Cuss Programming 2026.

#include "AI/ARPGEnemyAIController.h"

#include "Character/ARPGEnemyCharacter.h"
#include "Components/ARPGCombatantComponent.h"
#include "Components/ARPGCombatCoordinationComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Subsystems/ARPGCombatantRegistrySubsystem.h"

AARPGEnemyAIController::AARPGEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	BrainComponent = StateTreeComponent;

	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
}

void AARPGEnemyAIController::OnPossess(APawn* InPawn)
{
	const AARPGEnemyCharacter* EnemyCharacter = Cast<AARPGEnemyCharacter>(InPawn);

	if (IsValid(EnemyCharacter) && EnemyCharacter->GetStateTreeAsset())
	{
		StateTreeComponent->SetStateTree(EnemyCharacter->GetStateTreeAsset());
	}

	Super::OnPossess(InPawn);

	if (!HasAuthority())
	{
		return;
	}

	if (!ensureMsgf(IsValid(EnemyCharacter), TEXT("ARPGEnemyAIController must possess an ARPGEnemyCharacter.")))
	{
		return;
	}

	ControlledCombatantComponent = EnemyCharacter->GetCombatantComponent();
	CombatantRegistrySubsystem = GetWorld()->GetSubsystem<UARPGCombatantRegistrySubsystem>();

	if (!ensureMsgf(IsValid(ControlledCombatantComponent), TEXT("ARPGEnemyCharacter is missing its CombatantComponent.")))
	{
		return;
	}

	if (!ensureMsgf(IsValid(CombatantRegistrySubsystem), TEXT("ARPGCombatantRegistrySubsystem is unavailable.")))
	{
		return;
	}

	UpdateTarget();

	const float InitialDelay = TargetRefreshInterval + FMath::FRandRange(0.0f, TargetRefreshInterval);
	GetWorldTimerManager().SetTimer(TargetRefreshTimerHandle, this, &AARPGEnemyAIController::UpdateTarget, TargetRefreshInterval, true, InitialDelay);
}

void AARPGEnemyAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(TargetRefreshTimerHandle);

	SetCurrentTarget(nullptr);

	ControlledCombatantComponent = nullptr;
	CombatantRegistrySubsystem = nullptr;

	Super::OnUnPossess();
}

void AARPGEnemyAIController::UpdateTarget()
{
	if (!IsValid(ControlledCombatantComponent) || !IsValid(CombatantRegistrySubsystem))
	{
		return;
	}

	if (IsCurrentTargetValid())
	{
		return;
	}

	SetCurrentTarget(CombatantRegistrySubsystem->FindNearestHostile(ControlledCombatantComponent, TargetAcquisitionRadius));
}

void AARPGEnemyAIController::SetCurrentTarget(UARPGCombatantComponent* NewTarget)
{
	if (CurrentTargetCombatant.Get() == NewTarget)
	{
		return;
	}
	
	if (IsValid(TargetCoordinationComponent) && IsValid(ControlledCombatantComponent))
	{
		TargetCoordinationComponent->UnregisterAttacker(ControlledCombatantComponent);
	}

	TargetCoordinationComponent = nullptr;

	CurrentTargetCombatant = NewTarget;
	const AActor* CurrentTargetActor = NewTarget ? NewTarget->GetCombatantActor() : nullptr;

	if (IsValid(ControlledCombatantComponent))
	{
		ControlledCombatantComponent->SetCurrentTarget(NewTarget);
	}
	
	if (CurrentTargetActor && IsValid(ControlledCombatantComponent))
	{
		TargetCoordinationComponent = CurrentTargetActor->FindComponentByClass<UARPGCombatCoordinationComponent>();

		if (IsValid(TargetCoordinationComponent))
		{
			TargetCoordinationComponent->RegisterAttacker(ControlledCombatantComponent);
		}
	}
}

bool AARPGEnemyAIController::IsCurrentTargetValid() const
{
	const UARPGCombatantComponent* TargetCombatant = CurrentTargetCombatant.Get();

	if (!ControlledCombatantComponent || !IsValid(TargetCombatant) || !TargetCombatant->IsTargetable() || !ControlledCombatantComponent->IsHostileTo(TargetCombatant))
	{
		return false;
	}

	const AActor* ControlledActor = ControlledCombatantComponent->GetCombatantActor();
	const AActor* TargetActor = TargetCombatant->GetCombatantActor();

	if (!IsValid(ControlledActor) || !IsValid(TargetActor))
	{
		return false;
	}

	return FVector::DistSquared2D(ControlledActor->GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(TargetDropRadius);
}