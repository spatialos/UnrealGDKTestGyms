// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySpecHandleGymCharacter.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "Components/InputComponent.h"

void AAbilitySpecHandleGymCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::Q, EInputEvent::IE_Pressed, this, &AAbilitySpecHandleGymCharacter::GiveAbility);
	PlayerInputComponent->BindKey(EKeys::E, EInputEvent::IE_Pressed, this, &AAbilitySpecHandleGymCharacter::GiveAbilityWhileLocked);
}

void AAbilitySpecHandleGymCharacter::GiveAbility_Implementation()
{
	const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGameplayAbility::StaticClass()));
	UE_LOG(LogTemp, Log, TEXT("Got handle %s"), *Handle.ToString()); // TODO log category
	LogActivatableAbilities();
}

void AAbilitySpecHandleGymCharacter::GiveAbilityWhileLocked_Implementation()
{
	{
		// Lock the ASC ability list for this scope
		// Can't use ABILITYLIST_SCOPE_LOCK since that assumes that `this` is of type UAbilitySystemComponent*
		FScopedAbilityListLock Lock(*AbilitySystemComponent);
		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGameplayAbility::StaticClass()));
		UE_LOG(LogTemp, Log, TEXT("Got handle %s"), *Handle.ToString()); // TODO log category
	}

	// Log abilities after the scope lock is dropped, since the ability spec won't be added to the activatable abilities array before that.
	LogActivatableAbilities();
}

void AAbilitySpecHandleGymCharacter::LogActivatableAbilities() const
{
	UE_LOG(LogTemp, Log, TEXT("%s activatable abilities:"), *GetNameSafe(this));
	const TArray<FGameplayAbilitySpec>& Specs = AbilitySystemComponent->GetActivatableAbilities();
	for (auto Spec : Specs)
	{
		UE_LOG(LogTemp, Log, TEXT("  %s (%s)"), *GetNameSafe(Spec.Ability), *Spec.Handle.ToString());
	}
}
