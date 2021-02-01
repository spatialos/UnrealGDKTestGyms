// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecHandleTestCharacter.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "Components/InputComponent.h"

void ASpecHandleTestCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::T, EInputEvent::IE_Pressed, this, &ASpecHandleTestCharacter::GiveAbilityAndLogActivatableAbilities);
}

void ASpecHandleTestCharacter::GiveAbilityAndLogActivatableAbilities_Implementation()
{
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGameplayAbility::StaticClass()));

	const TArray<FGameplayAbilitySpec>& Specs = AbilitySystemComponent->GetActivatableAbilities();

	UE_LOG(LogTemp, Log, TEXT("%s activatable abilities:"), *GetNameSafe(this));
	for (auto Spec : Specs)
	{
		UE_LOG(LogTemp, Log, TEXT("  %s (%s)"), *GetNameSafe(Spec.Ability), *Spec.Handle.ToString());
	}
}
