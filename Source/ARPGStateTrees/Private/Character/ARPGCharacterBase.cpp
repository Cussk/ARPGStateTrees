// Copyright Kyle Cuss and Cuss Programming 2026.

#include "Character/ARPGCharacterBase.h"

#include "Components/ARPGCombatantComponent.h"

AARPGCharacterBase::AARPGCharacterBase()
{
	CombatantComponent = CreateDefaultSubobject<UARPGCombatantComponent>(TEXT("CombatantComponent"));
}

UARPGCombatantComponent* AARPGCharacterBase::GetCombatantComponent() const
{
	return CombatantComponent;
}

float AARPGCharacterBase::PlayMontage(UAnimMontage* Montage, FOnMontageBlendingOutStarted& BlendOutDelegate, const float PlayRate) const
{
	if (!IsValid(Montage) || !IsValid(GetMesh()))
	{
		return 0.0f;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return 0.0f;
	}

	const float Duration = AnimInstance->Montage_Play(Montage, PlayRate);

	if (Duration <= 0.0f)
	{
		return 0.0f;
	}

	AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);

	return Duration;
}