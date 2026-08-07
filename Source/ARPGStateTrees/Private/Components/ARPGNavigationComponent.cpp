// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/ARPGNavigationComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

UARPGNavigationComponent::UARPGNavigationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UARPGNavigationComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!ensureMsgf(OwnerCharacter,
		TEXT("ARPGNavigationComponent must be owned by an ACharacter.")))
	{
		return;
	}

	CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();

	if (!ensureMsgf(CharacterMovementComponent,
		TEXT("ARPGNavigationComponent requires a CharacterMovementComponent.")))
	{
		return;
	}

	// Navigation input needs to be supplied before CharacterMovement consumes it.
	CharacterMovementComponent->AddTickPrerequisiteComponent(this);
}

void UARPGNavigationComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		CompleteNavigation();
		return;
	}

	if (!PathPoints.IsValidIndex(CurrentPathPointIndex))
	{
		CompleteNavigation();
		return;
	}

	AdvancePathIfNeeded();

	if (!PathPoints.IsValidIndex(CurrentPathPointIndex))
	{
		return;
	}

	FVector MoveDirection =
		PathPoints[CurrentPathPointIndex] - OwnerCharacter->GetActorLocation();

	MoveDirection.Z = 0.0f;

	if (!MoveDirection.Normalize())
	{
		return;
	}

	OwnerCharacter->AddMovementInput(MoveDirection);
}

bool UARPGNavigationComponent::RequestMoveToLocation(const FVector& WorldDestination)
{
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return false;
	}

	RecordDestinationRequest(WorldDestination);

	if (!TryBuildPath(WorldDestination))
	{
		// A new click takes precedence over the player's previous intent.
		CancelNavigation();
		return false;
	}

	return true;
}

bool UARPGNavigationComponent::UpdateMoveDestination(const FVector& WorldDestination)
{
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return false;
	}

	if (!ShouldUpdateDestination(WorldDestination))
	{
		return false;
	}

	RecordDestinationRequest(WorldDestination);

	// Unlike a discrete click, an invalid held-movement update should not
	// destroy an otherwise valid current path.
	return TryBuildPath(WorldDestination);
}

void UARPGNavigationComponent::CancelNavigation()
{
	PathPoints.Reset();
	CurrentPathPointIndex = INDEX_NONE;

	SetComponentTickEnabled(false);
}

bool UARPGNavigationComponent::IsNavigating() const
{
	return PathPoints.IsValidIndex(CurrentPathPointIndex);
}

bool UARPGNavigationComponent::TryBuildPath(const FVector& WorldDestination)
{
	if (!OwnerCharacter || !CharacterMovementComponent)
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	if (!NavigationSystem)
	{
		return false;
	}

	FNavLocation ProjectedDestination;

	const FNavAgentProperties& AgentProperties = CharacterMovementComponent->GetNavAgentPropertiesRef();

	if (!NavigationSystem->ProjectPointToNavigation(
		WorldDestination,
		ProjectedDestination,
		NavigationProjectionExtent,
		&AgentProperties))
	{
		return false;
	}

	const FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	if (FVector::DistSquared2D(
			CharacterLocation,
			ProjectedDestination.Location)
		<= FMath::Square(FinalDestinationAcceptanceRadius))
	{
		ResolvedDestination = ProjectedDestination.Location;
		CompleteNavigation();
		return true;
	}

	UNavigationPath* NavigationPath =
		UNavigationSystemV1::FindPathToLocationSynchronously(
			this,
			CharacterLocation,
			ProjectedDestination.Location,
			OwnerCharacter);

	if (!NavigationPath
		|| !NavigationPath->IsValid()
		|| NavigationPath->IsPartial()
		|| NavigationPath->PathPoints.Num() < 2)
	{
		return false;
	}
	
	PathPoints = NavigationPath->PathPoints;
	CurrentPathPointIndex = 1;

	ResolvedDestination = PathPoints.Last();

	SetComponentTickEnabled(true);

	return true;
}

bool UARPGNavigationComponent::ShouldUpdateDestination(
	const FVector& WorldDestination) const
{
	if (!bHasDestinationRequest)
	{
		return true;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const double TimeSinceLastRequest =
		World->GetTimeSeconds() - LastDestinationRequestTime;

	if (TimeSinceLastRequest < MinimumDestinationUpdateInterval)
	{
		return false;
	}

	const float DistanceSquared =
		FVector::DistSquared2D(
			WorldDestination,
			LastRequestedDestination);

	return DistanceSquared >= FMath::Square(DestinationChangeThreshold);
}

void UARPGNavigationComponent::RecordDestinationRequest(
	const FVector& WorldDestination)
{
	LastRequestedDestination = WorldDestination;
	bHasDestinationRequest = true;

	if (const UWorld* World = GetWorld())
	{
		LastDestinationRequestTime = World->GetTimeSeconds();
	}
}

void UARPGNavigationComponent::AdvancePathIfNeeded()
{
	if (!OwnerCharacter)
	{
		CompleteNavigation();
		return;
	}

	const FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	while (PathPoints.IsValidIndex(CurrentPathPointIndex))
	{
		const bool bIsFinalPoint = CurrentPathPointIndex == PathPoints.Num() - 1;

		const float AcceptanceRadius = bIsFinalPoint ? FinalDestinationAcceptanceRadius : IntermediatePointAcceptanceRadius;

		const float DistanceSquared = FVector::DistSquared2D(CharacterLocation, PathPoints[CurrentPathPointIndex]);

		if (DistanceSquared > FMath::Square(AcceptanceRadius))
		{
			return;
		}

		if (bIsFinalPoint)
		{
			CompleteNavigation();
			return;
		}

		++CurrentPathPointIndex;
	}
}

void UARPGNavigationComponent::CompleteNavigation()
{
	PathPoints.Reset();
	CurrentPathPointIndex = INDEX_NONE;

	SetComponentTickEnabled(false);
}