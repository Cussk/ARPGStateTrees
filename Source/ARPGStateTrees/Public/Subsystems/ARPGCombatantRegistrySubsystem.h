// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARPGCombatantRegistrySubsystem.generated.h"

class UARPGCombatantComponent;

/**
 * Maintains the server-side set of active combatants and provides lightweight combatant lookup.
 */
UCLASS()
class ARPGSTATETREES_API UARPGCombatantRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterCombatant(UARPGCombatantComponent* Combatant);
	void UnregisterCombatant(UARPGCombatantComponent* Combatant);

	UARPGCombatantComponent* FindNearestHostile(const UARPGCombatantComponent* Requester, float SearchRadius) const;
	UARPGCombatantComponent* FindNearestHostileToLocation(const UARPGCombatantComponent* Requester,	const FVector& SearchOrigin, float SearchRadius) const;

	int32 GetCombatantCount() const;

protected:
	TSet<TWeakObjectPtr<UARPGCombatantComponent>> Combatants;
};