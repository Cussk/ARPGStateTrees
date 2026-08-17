// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Debug/ARPGAIStressDirector.h"

#include "Character/ARPGCompanionCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Character/ARPGPlayerCharacter.h"
#include "Components/ARPGCompanionComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AARPGAIStressDirector::AARPGAIStressDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AARPGAIStressDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoStart || !IsServerGameWorld())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		AutoStartTimer,
		this,
		&AARPGAIStressDirector::StartStressTest,
		AutoStartDelay,
		false);
}

void AARPGAIStressDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
	GetWorldTimerManager().ClearTimer(AutoStartTimer);

	Super::EndPlay(EndPlayReason);
}

void AARPGAIStressDirector::StartStressTest()
{
	if (!IsServerGameWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Stress Director: Stress tests can only be started in the server game world."));
		return;
	}

	ClearStressActors();

	NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!IsValid(NavigationSystem))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Stress Director: Navigation system unavailable."));
		return;
	}

	TArray<AARPGPlayerCharacter*> Players;
	GatherPlayerTargets(Players);

	if (Players.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Stress Director: No player characters found."));
		return;
	}

	SpawnRandom.Initialize(RandomSeed);

	TArray<TSubclassOf<AARPGEnemyCharacter>> EnemyClasses;

	if (TotalEnemyCount > 0 && !BuildEnemyClassSequence(EnemyClasses))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Stress Director: No valid enemy mix configured."));
		return;
	}

	BuildEnemySpawnRequests(Players, EnemyClasses);
	SpawnCompanions(Players);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("AI Stress Director: Starting %d enemies, %d companions per player, %d players."),
		TotalEnemyCount,
		CompanionsPerPlayer,
		Players.Num());

	if (PendingEnemySpawns.IsEmpty())
	{
		return;
	}

	SpawnNextEnemyBatch();

	if (PendingEnemySpawnIndex < PendingEnemySpawns.Num())
	{
		GetWorldTimerManager().SetTimer(
			EnemySpawnTimer,
			this,
			&AARPGAIStressDirector::SpawnNextEnemyBatch,
			SpawnBatchInterval,
			true);
	}
}

void AARPGAIStressDirector::StopStressTest()
{
	GetWorldTimerManager().ClearTimer(EnemySpawnTimer);

	PendingEnemySpawns.Reset();
	PendingEnemySpawnIndex = 0;
}

void AARPGAIStressDirector::ClearStressActors()
{
	StopStressTest();

	for (const TWeakObjectPtr<AActor>& WeakActor : SpawnedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Actor->Destroy();
		}
	}

	SpawnedActors.Reset();

	SpawnedEnemyCount = 0;
	FailedEnemySpawnCount = 0;
	SpawnedCompanionCount = 0;
}

void AARPGAIStressDirector::GatherPlayerTargets(TArray<AARPGPlayerCharacter*>& OutPlayers) const
{
	OutPlayers.Reset();

	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();

		if (!IsValid(PlayerController))
		{
			continue;
		}

		AARPGPlayerCharacter* PlayerCharacter = Cast<AARPGPlayerCharacter>(PlayerController->GetPawn());

		if (IsValid(PlayerCharacter))
		{
			OutPlayers.AddUnique(PlayerCharacter);
		}
	}

	OutPlayers.Sort([](const AARPGPlayerCharacter& A, const AARPGPlayerCharacter& B)
	{
		const APlayerState* AState = A.GetPlayerState();
		const APlayerState* BState = B.GetPlayerState();

		const int32 APlayerId = IsValid(AState) ? AState->GetPlayerId() : MAX_int32;
		const int32 BPlayerId = IsValid(BState) ? BState->GetPlayerId() : MAX_int32;

		if (APlayerId != BPlayerId)
		{
			return APlayerId < BPlayerId;
		}

		return A.GetName() < B.GetName();
	});
}

bool AARPGAIStressDirector::BuildEnemyClassSequence(TArray<TSubclassOf<AARPGEnemyCharacter>>& OutClasses)
{
	OutClasses.Reset();

	if (TotalEnemyCount <= 0)
	{
		return true;
	}

	struct FResolvedMixEntry
	{
		TSubclassOf<AARPGEnemyCharacter> EnemyClass;
		int32 Count = 0;
		float Remainder = 0.0f;
	};

	float TotalWeight = 0.0f;

	for (const FARPGStressEnemyMixEntry& Entry : EnemyMix)
	{
		if (Entry.EnemyClass && Entry.Weight > 0.0f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return false;
	}

	TArray<FResolvedMixEntry> ResolvedMix;
	int32 AssignedCount = 0;

	for (const FARPGStressEnemyMixEntry& Entry : EnemyMix)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0.0f)
		{
			continue;
		}

		const float ExactCount = static_cast<float>(TotalEnemyCount) * Entry.Weight / TotalWeight;
		const int32 BaseCount = FMath::FloorToInt(ExactCount);

		FResolvedMixEntry& ResolvedEntry = ResolvedMix.AddDefaulted_GetRef();
		ResolvedEntry.EnemyClass = Entry.EnemyClass;
		ResolvedEntry.Count = BaseCount;
		ResolvedEntry.Remainder = ExactCount - static_cast<float>(BaseCount);

		AssignedCount += BaseCount;
	}

	ResolvedMix.Sort([](const FResolvedMixEntry& A, const FResolvedMixEntry& B)
	{
		return A.Remainder > B.Remainder;
	});

	const int32 RemainingCount = TotalEnemyCount - AssignedCount;

	for (int32 Index = 0; Index < RemainingCount; ++Index)
	{
		++ResolvedMix[Index % ResolvedMix.Num()].Count;
	}

	OutClasses.Reserve(TotalEnemyCount);

	for (const FResolvedMixEntry& Entry : ResolvedMix)
	{
		for (int32 Count = 0; Count < Entry.Count; ++Count)
		{
			OutClasses.Add(Entry.EnemyClass);
		}
	}

	for (int32 Index = 0; Index < OutClasses.Num() - 1; ++Index)
	{
		const int32 SwapIndex = SpawnRandom.RandRange(Index, OutClasses.Num() - 1);
		OutClasses.Swap(Index, SwapIndex);
	}

	return !OutClasses.IsEmpty();
}

void AARPGAIStressDirector::BuildEnemySpawnRequests(const TArray<AARPGPlayerCharacter*>& Players,
	const TArray<TSubclassOf<AARPGEnemyCharacter>>& EnemyClasses)
{
	PendingEnemySpawns.Reset();
	PendingEnemySpawnIndex = 0;

	if (Players.IsEmpty())
	{
		return;
	}

	PendingEnemySpawns.Reserve(EnemyClasses.Num());

	const int32 ConcentratedIndex = FMath::Clamp(ConcentratedPlayerIndex, 0, Players.Num() - 1);

	for (int32 Index = 0; Index < EnemyClasses.Num(); ++Index)
	{
		const int32 PlayerIndex = Distribution == EARPGStressDistribution::Concentrated
			? ConcentratedIndex
			: Index % Players.Num();

		FEnemySpawnRequest& Request = PendingEnemySpawns.AddDefaulted_GetRef();
		Request.AnchorPlayer = Players[PlayerIndex];
		Request.EnemyClass = EnemyClasses[Index];
	}
}

void AARPGAIStressDirector::SpawnCompanions(const TArray<AARPGPlayerCharacter*>& Players)
{
	if (CompanionsPerPlayer <= 0 || !CompanionClass)
	{
		return;
	}

	for (AARPGPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		for (int32 Index = 0; Index < CompanionsPerPlayer; ++Index)
		{
			FVector SpawnLocation;

			if (!FindSpawnLocation(Player, CompanionSpawnRadiusMin, CompanionSpawnRadiusMax, SpawnLocation))
			{
				continue;
			}

			FRotator SpawnRotation = (Player->GetActorLocation() - SpawnLocation).Rotation();
			SpawnRotation.Pitch = 0.0f;
			SpawnRotation.Roll = 0.0f;

			const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

			AARPGCompanionCharacter* Companion = GetWorld()->SpawnActorDeferred<AARPGCompanionCharacter>(
				CompanionClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

			if (!IsValid(Companion))
			{
				continue;
			}

			if (UARPGCompanionComponent* CompanionComponent = Companion->GetCompanionComponent())
			{
				CompanionComponent->SetCompanionOwner(Player);
			}

			UGameplayStatics::FinishSpawningActor(Companion, SpawnTransform);

			SpawnedActors.Add(Companion);
			++SpawnedCompanionCount;
		}
	}
}

void AARPGAIStressDirector::SpawnNextEnemyBatch()
{
	const int32 BatchEnd = FMath::Min(
		PendingEnemySpawnIndex + FMath::Max(1, SpawnBatchSize),
		PendingEnemySpawns.Num());

	while (PendingEnemySpawnIndex < BatchEnd)
	{
		if (AARPGEnemyCharacter* Enemy = SpawnEnemy(PendingEnemySpawns[PendingEnemySpawnIndex]))
		{
			SpawnedActors.Add(Enemy);
			++SpawnedEnemyCount;
		}
		else
		{
			++FailedEnemySpawnCount;
		}

		++PendingEnemySpawnIndex;
	}

	if (PendingEnemySpawnIndex < PendingEnemySpawns.Num())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(EnemySpawnTimer);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("AI Stress Director: Spawn complete. Enemies=%d Failed=%d Companions=%d"),
		SpawnedEnemyCount,
		FailedEnemySpawnCount,
		SpawnedCompanionCount);
}

bool AARPGAIStressDirector::FindSpawnLocation(const AActor* Anchor, const float MinRadius, const float MaxRadius,
	FVector& OutLocation)
{
	if (!IsValid(Anchor) || !IsValid(NavigationSystem))
	{
		return false;
	}

	const float SafeMinRadius = FMath::Max(0.0f, MinRadius);
	const float SafeMaxRadius = FMath::Max(SafeMinRadius, MaxRadius);
	const FVector AnchorLocation = Anchor->GetActorLocation();

	for (int32 Attempt = 0; Attempt < MaxSpawnLocationAttempts; ++Attempt)
	{
		const float Angle = SpawnRandom.FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::Sqrt(SpawnRandom.FRandRange(
			FMath::Square(SafeMinRadius),
			FMath::Square(SafeMaxRadius)));

		const FVector CandidateLocation = AnchorLocation
			+ FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;

		FNavLocation ProjectedLocation;

		if (NavigationSystem->ProjectPointToNavigation(CandidateLocation, ProjectedLocation, SpawnProjectionExtent))
		{
			OutLocation = ProjectedLocation.Location;
			return true;
		}
	}

	return false;
}

AARPGEnemyCharacter* AARPGAIStressDirector::SpawnEnemy(const FEnemySpawnRequest& Request)
{
	AARPGPlayerCharacter* AnchorPlayer = Request.AnchorPlayer.Get();

	if (!IsValid(AnchorPlayer) || !Request.EnemyClass)
	{
		return nullptr;
	}

	FVector SpawnLocation;

	if (!FindSpawnLocation(AnchorPlayer, EnemySpawnRadiusMin, EnemySpawnRadiusMax, SpawnLocation))
	{
		return nullptr;
	}

	FRotator SpawnRotation = (AnchorPlayer->GetActorLocation() - SpawnLocation).Rotation();
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return GetWorld()->SpawnActor<AARPGEnemyCharacter>(
		Request.EnemyClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);
}

bool AARPGAIStressDirector::IsServerGameWorld() const
{
	const UWorld* World = GetWorld();

	return World
		&& World->IsGameWorld()
		&& World->GetNetMode() != NM_Client;
}