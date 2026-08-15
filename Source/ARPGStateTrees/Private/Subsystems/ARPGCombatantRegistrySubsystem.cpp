// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Subsystems/ARPGCombatantRegistrySubsystem.h"

#include "Components/ARPGCombatantComponent.h"

void UARPGCombatantRegistrySubsystem::RegisterCombatant(UARPGCombatantComponent* Combatant)
{
	if (IsValid(Combatant))
	{
		Combatants.Add(Combatant);
	}
}

void UARPGCombatantRegistrySubsystem::UnregisterCombatant(UARPGCombatantComponent* Combatant)
{
	if (IsValid(Combatant))
	{
		Combatants.Remove(Combatant);
	}
}

UARPGCombatantComponent* UARPGCombatantRegistrySubsystem::FindNearestHostile(
	const UARPGCombatantComponent* Requester,
	const float SearchRadius) const
{
	if (!IsValid(Requester) || !IsValid(Requester->GetCombatantActor()))
	{
		return nullptr;
	}

	return FindNearestHostileToLocation(Requester, Requester->GetCombatantActor()->GetActorLocation(), SearchRadius);
}

UARPGCombatantComponent* UARPGCombatantRegistrySubsystem::FindNearestHostileToLocation(
	const UARPGCombatantComponent* Requester,
	const FVector& SearchOrigin,
	const float SearchRadius) const
{
	if (!IsValid(Requester))
	{
		return nullptr;
	}

	UARPGCombatantComponent* BestCombatant = nullptr;
	float BestDistanceSquared = FMath::Square(SearchRadius);

	for (const TWeakObjectPtr<UARPGCombatantComponent>& WeakCombatant : Combatants)
	{
		UARPGCombatantComponent* Combatant = WeakCombatant.Get();

		if (!IsValid(Combatant) || Combatant == Requester || !Combatant->IsTargetable()
			|| !Requester->IsHostileTo(Combatant))
		{
			continue;
		}

		const AActor* CombatantActor = Combatant->GetCombatantActor();

		if (!IsValid(CombatantActor))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(SearchOrigin, CombatantActor->GetActorLocation());

		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		BestCombatant = Combatant;
	}

	return BestCombatant;
}

int32 UARPGCombatantRegistrySubsystem::GetCombatantCount() const
{
	return Combatants.Num();
}
