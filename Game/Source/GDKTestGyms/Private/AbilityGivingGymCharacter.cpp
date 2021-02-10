// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "AbilityGivingGymCharacter.h"
#include "GameplayAbilitySpec.h"
#include "Components/InputComponent.h"

void AAbilityGivingGymCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::Q, EInputEvent::IE_Pressed, this, &AAbilityGivingGymCharacter::GiveAbilityOne);
	PlayerInputComponent->BindKey(EKeys::E, EInputEvent::IE_Pressed, this, &AAbilityGivingGymCharacter::GiveAbilityTwo);
}

void AAbilityGivingGymCharacter::ServerGiveAbility_Implementation(int32 Level)
{
	UE_LOG(LogTemp, Log, TEXT("Giving and running ability with level %d"), Level);
	FGameplayAbilitySpec Spec(AbilityToAdd, Level);
	FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
	AbilitySystemComponent->TryActivateAbility(Handle);
}
