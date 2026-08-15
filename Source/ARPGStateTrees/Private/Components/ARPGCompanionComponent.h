// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGCompanionComponent.generated.h"

class UARPGCombatantComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FARPGCompanionOwnerChanged, AActor*);

UCLASS(ClassGroup = (ARPG), meta = (BlueprintSpawnableComponent))
class ARPGSTATETREES_API UARPGCompanionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGCompanionComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SetCompanionOwner(AActor* NewOwner);

	UFUNCTION(BlueprintPure, Category = "Companion")
	AActor* GetCompanionOwnerActor() const;

	UFUNCTION(BlueprintPure, Category = "Companion")
	UARPGCombatantComponent* GetCompanionOwnerCombatant() const;

	float GetFollowDistance() const;
	float GetCatchUpDistance() const;
	float GetMaximumLeashDistance() const;
	float GetFollowUpdateInterval() const;

	FARPGCompanionOwnerChanged OnCompanionOwnerChanged;

protected:
	UFUNCTION()
	void OnRep_CompanionOwnerActor();

	void RefreshOwnerReferences();

	UPROPERTY(ReplicatedUsing = OnRep_CompanionOwnerActor, Transient, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<AActor> CompanionOwnerActor;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<UARPGCombatantComponent> CompanionOwnerCombatant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Movement", meta = (ClampMin = "0.0"))
	float FollowDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Movement", meta = (ClampMin = "0.0"))
	float CatchUpDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Movement", meta = (ClampMin = "0.0"))
	float MaximumLeashDistance = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Movement", meta = (ClampMin = "0.05"))
	float FollowUpdateInterval = 0.25f;
};

