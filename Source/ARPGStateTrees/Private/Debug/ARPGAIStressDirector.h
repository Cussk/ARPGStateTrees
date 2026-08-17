// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGAIStressDirector.generated.h"

class AARPGCompanionCharacter;
class AARPGEnemyCharacter;
class AARPGPlayerCharacter;
class UNavigationSystemV1;

UENUM(BlueprintType)
enum class EARPGStressDistribution : uint8
{
	Concentrated,
	Distributed
};

USTRUCT(BlueprintType)
struct FARPGStressEnemyMixEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test")
	TSubclassOf<AARPGEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

/**
 * Debug-only runtime harness for repeatable AI population and multiplayer stress testing.
 */
UCLASS()
class ARPGSTATETREES_API AARPGAIStressDirector : public AActor
{
	GENERATED_BODY()

public:
	AARPGAIStressDirector();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stress Test")
	void StartStressTest();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stress Test")
	void StopStressTest();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stress Test")
	void ClearStressActors();

protected:
	struct FEnemySpawnRequest
	{
		TWeakObjectPtr<AARPGPlayerCharacter> AnchorPlayer;
		TSubclassOf<AARPGEnemyCharacter> EnemyClass;
	};

	void GatherPlayerTargets(TArray<AARPGPlayerCharacter*>& OutPlayers) const;
	bool BuildEnemyClassSequence(TArray<TSubclassOf<AARPGEnemyCharacter>>& OutClasses);
	void BuildEnemySpawnRequests(const TArray<AARPGPlayerCharacter*>& Players, const TArray<TSubclassOf<AARPGEnemyCharacter>>& EnemyClasses);
	void SpawnCompanions(const TArray<AARPGPlayerCharacter*>& Players);
	void SpawnNextEnemyBatch();

	bool FindSpawnLocation(const AActor* Anchor, float MinRadius, float MaxRadius, FVector& OutLocation);
	AARPGEnemyCharacter* SpawnEnemy(const FEnemySpawnRequest& Request);

	bool IsServerGameWorld() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario", meta = (ClampMin = "0"))
	int32 TotalEnemyCount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario", meta = (ClampMin = "0"))
	int32 CompanionsPerPlayer = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario")
	EARPGStressDistribution Distribution = EARPGStressDistribution::Concentrated;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario", meta = (ClampMin = "0"))
	int32 ConcentratedPlayerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario")
	TArray<FARPGStressEnemyMixEntry> EnemyMix;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Scenario")
	TSubclassOf<AARPGCompanionCharacter> CompanionClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "0.0"))
	float EnemySpawnRadiusMin = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "0.0"))
	float EnemySpawnRadiusMax = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "0.0"))
	float CompanionSpawnRadiusMin = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "0.0"))
	float CompanionSpawnRadiusMax = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "1"))
	int32 SpawnBatchSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "0.01"))
	float SpawnBatchInterval = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning", meta = (ClampMin = "1"))
	int32 MaxSpawnLocationAttempts = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Spawning")
	FVector SpawnProjectionExtent = FVector(150.0f, 150.0f, 500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Automation")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stress Test|Automation", meta = (ClampMin = "0.0"))
	float AutoStartDelay = 2.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stress Test|Runtime")
	int32 SpawnedEnemyCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stress Test|Runtime")
	int32 FailedEnemySpawnCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stress Test|Runtime")
	int32 SpawnedCompanionCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UNavigationSystemV1> NavigationSystem;

	TArray<FEnemySpawnRequest> PendingEnemySpawns;
	TArray<TWeakObjectPtr<AActor>> SpawnedActors;

	FRandomStream SpawnRandom;

	FTimerHandle EnemySpawnTimer;
	FTimerHandle AutoStartTimer;

	int32 PendingEnemySpawnIndex = 0;
};