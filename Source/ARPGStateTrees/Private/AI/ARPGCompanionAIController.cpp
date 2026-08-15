// Copyright Kyle Cuss and Cuss Programming 2026.


#include "AI/ARPGCompanionAIController.h"

#include "Character/ARPGCompanionCharacter.h"
#include "Components/ARPGCombatantComponent.h"
#include "Components/ARPGCombatCoordinationComponent.h"
#include "Components/ARPGCompanionComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Subsystems/ARPGCombatantRegistrySubsystem.h"

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

	if (!HasAuthority())
	{
		return;
	}

	if (!ensureMsgf(IsValid(CompanionCharacter), TEXT("ARPGCompanionAIController must possess an ARPGCompanionCharacter.")))
	{
		return;
	}

	ControlledCombatantComponent = CompanionCharacter->GetCombatantComponent();
	CompanionComponent = CompanionCharacter->GetCompanionComponent();
	CombatantRegistrySubsystem = GetWorld()->GetSubsystem<UARPGCombatantRegistrySubsystem>();

	if (!ensureMsgf(IsValid(ControlledCombatantComponent), TEXT("ARPGCompanionCharacter is missing its CombatantComponent.")))
	{
		return;
	}

	if (!ensureMsgf(IsValid(CompanionComponent), TEXT("ARPGCompanionCharacter is missing its CompanionComponent.")))
	{
		return;
	}

	if (!ensureMsgf(IsValid(CombatantRegistrySubsystem), TEXT("ARPGCombatantRegistrySubsystem is unavailable.")))
	{
		return;
	}

	OwnerChangedHandle = CompanionComponent->OnCompanionOwnerChanged.AddUObject(
		this, &AARPGCompanionAIController::HandleCompanionOwnerChanged);

	UpdateTarget();

	const float InitialDelay = TargetRefreshInterval + FMath::FRandRange(0.0f, TargetRefreshInterval);

	GetWorldTimerManager().SetTimer(
		TargetRefreshTimerHandle,
		this,
		&AARPGCompanionAIController::UpdateTarget,
		TargetRefreshInterval,
		true,
		InitialDelay);
}

void AARPGCompanionAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(TargetRefreshTimerHandle);

	if (IsValid(CompanionComponent) && OwnerChangedHandle.IsValid())
	{
		CompanionComponent->OnCompanionOwnerChanged.Remove(OwnerChangedHandle);
	}

	OwnerChangedHandle.Reset();

	SetCurrentTarget(nullptr);

	ControlledCombatantComponent = nullptr;
	CompanionComponent = nullptr;
	CombatantRegistrySubsystem = nullptr;

	Super::OnUnPossess();
}

void AARPGCompanionAIController::UpdateTarget()
{
	if (!IsValid(ControlledCombatantComponent) || !IsValid(CompanionComponent) || !IsValid(CombatantRegistrySubsystem))
	{
		return;
	}

	if (IsCurrentTargetValid())
	{
		return;
	}

	SetCurrentTarget(nullptr);

	AActor* OwnerActor = CompanionComponent->GetCompanionOwnerActor();

	if (!IsValid(OwnerActor))
	{
		return;
	}

	const float SearchRadius = FMath::Min(
		TargetAcquisitionRadius,
		CompanionComponent->GetMaximumLeashDistance());

	UARPGCombatantComponent* NewTarget = CombatantRegistrySubsystem->FindNearestHostileToLocation(
		ControlledCombatantComponent,
		OwnerActor->GetActorLocation(),
		SearchRadius);

	SetCurrentTarget(NewTarget);
}

void AARPGCompanionAIController::SetCurrentTarget(UARPGCombatantComponent* NewTarget)
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

	if (IsValid(ControlledCombatantComponent))
	{
		ControlledCombatantComponent->SetCurrentTarget(NewTarget);
	}

	AActor* TargetActor = IsValid(NewTarget) ? NewTarget->GetCombatantActor() : nullptr;

	if (!IsValid(TargetActor) || !IsValid(ControlledCombatantComponent))
	{
		return;
	}

	TargetCoordinationComponent = TargetActor->FindComponentByClass<UARPGCombatCoordinationComponent>();

	if (IsValid(TargetCoordinationComponent))
	{
		TargetCoordinationComponent->RegisterAttacker(ControlledCombatantComponent);
	}
}

bool AARPGCompanionAIController::IsCurrentTargetValid() const
{
	const UARPGCombatantComponent* TargetCombatant = CurrentTargetCombatant.Get();

	if (!IsValid(ControlledCombatantComponent) || !IsValid(TargetCombatant)
		|| !TargetCombatant->IsTargetable()
		|| !ControlledCombatantComponent->IsHostileTo(TargetCombatant))
	{
		return false;
	}

	const AActor* ControlledActor = ControlledCombatantComponent->GetCombatantActor();
	const AActor* TargetActor = TargetCombatant->GetCombatantActor();

	if (!IsValid(ControlledActor) || !IsValid(TargetActor))
	{
		return false;
	}

	if (!IsWithinOwnerLeash(ControlledActor) || !IsWithinOwnerLeash(TargetActor))
	{
		return false;
	}

	return FVector::DistSquared2D(ControlledActor->GetActorLocation(), TargetActor->GetActorLocation())
		<= FMath::Square(TargetDropRadius);
}

bool AARPGCompanionAIController::IsWithinOwnerLeash(const AActor* Actor) const
{
	if (!IsValid(CompanionComponent) || !IsValid(Actor))
	{
		return false;
	}

	const AActor* OwnerActor = CompanionComponent->GetCompanionOwnerActor();

	if (!IsValid(OwnerActor))
	{
		return false;
	}

	return FVector::DistSquared2D(OwnerActor->GetActorLocation(), Actor->GetActorLocation())
		<= FMath::Square(CompanionComponent->GetMaximumLeashDistance());
}

void AARPGCompanionAIController::HandleCompanionOwnerChanged(AActor* NewOwner)
{
	SetCurrentTarget(nullptr);

	if (IsValid(NewOwner))
	{
		UpdateTarget();
	}
}

