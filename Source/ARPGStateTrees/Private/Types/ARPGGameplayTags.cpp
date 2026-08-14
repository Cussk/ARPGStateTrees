// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Types/ARPGGameplayTags.h"

namespace ARPGGameplayTags
{
	// STATE TREE EVENTS //
	UE_DEFINE_GAMEPLAY_TAG(StateTreeEvent_CombatTargetChanged, "StateTreeEvent.Combat.TargetChanged");
	UE_DEFINE_GAMEPLAY_TAG(StateTreeEvent_CombatRoleChanged, "StateTreeEvent.Combat.RoleChanged");
	UE_DEFINE_GAMEPLAY_TAG(StateTreeEvent_CombatGoalChanged, "StateTreeEvent.Combat.GoalChanged");
	UE_DEFINE_GAMEPLAY_TAG(StateTreeEvent_AttackOpportunity, "StateTreeEvent.Combat.AttackOpportunity");
	UE_DEFINE_GAMEPLAY_TAG(StateTreeEvent_SupportOpportunity, "StateTreeEvent.Combat.SupportOpportunity");
	UE_DEFINE_GAMEPLAY_TAG(StateTreeEvent_RangedPotshot, "StateTreeEvent.Combat.RangedPotshot");
}