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
enum class EARPGEngagementState : uint8
{
	None,
	Pressure,
	Engaged
};