// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGCrowdMovementComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup = "ARPG", meta = (BlueprintSpawnableComponent))
class ARPGSTATETREES_API UARPGCrowdMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGCrowdMovementComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetRightOfWayRequested(bool bRequested);
	bool IsRightOfWayRequested() const;

	void SetPassThroughActors(const TSet<AActor*>& DesiredActors);
	void RemovePassThroughActor(AActor* Actor);
	void ClearPassThroughActors();

protected:
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	TSet<TWeakObjectPtr<AActor>> PassThroughActors;

	bool bRightOfWayRequested = false;
};