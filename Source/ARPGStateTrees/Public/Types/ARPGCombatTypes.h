// Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EARPGCombatTeam : uint8
{
	Neutral,
	Player,
	Enemy
};

UENUM(BlueprintType)
enum class EARPGCoordinationState : uint8
{
	None,
	MeleePressure,
	MeleeEngaged,
	Ranged,
	Support
};

UENUM(BlueprintType)
enum class EARPGPositioningMode : uint8
{
	Melee,
	Ranged,
	Support
};