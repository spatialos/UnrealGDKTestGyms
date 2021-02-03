// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySpecHandleGymCharacter.h"
#include "GameplayAbilitySpec.h"
#include "Components/InputComponent.h"

void AAbilitySpecHandleGymCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::R, EInputEvent::IE_Pressed, this, &AAbilitySpecHandleGymCharacter::GiveAbility);
	PlayerInputComponent->BindKey(EKeys::T, EInputEvent::IE_Pressed, this, &AAbilitySpecHandleGymCharacter::GiveAbilityWhileLocked);

	PlayerInputComponent->BindKey(EKeys::F, EInputEvent::IE_Pressed, this, &AAbilitySpecHandleGymCharacter::GiveAbilityAndActivateOnce);
	PlayerInputComponent->BindKey(EKeys::G, EInputEvent::IE_Pressed, this, &AAbilitySpecHandleGymCharacter::GiveAbilityAndActivateOnceWhileLocked);
}

static void GiveAbilityUtil(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UGameplayAbility> AbilityToAdd, bool bActivate)
{
	const FGameplayAbilitySpec Spec(AbilityToAdd);
	FGameplayAbilitySpecHandle Handle;
	if (bActivate)
	{
		Handle = AbilitySystemComponent->GiveAbilityAndActivateOnce(Spec);
	} else
	{
		Handle = AbilitySystemComponent->GiveAbility(Spec);
	}
	UE_LOG(LogTemp, Log, TEXT("Got handle %s"), *Handle.ToString()); // TODO log category
}

void AAbilitySpecHandleGymCharacter::ServerGiveAbility_Implementation(bool bActivate, bool bLock)
{
	if (bLock)
	{
		// Lock the ASC ability list for this scope
		// Can't use ABILITYLIST_SCOPE_LOCK since that assumes that `this` is of type UAbilitySystemComponent*
		FScopedAbilityListLock Lock(*AbilitySystemComponent);
		GiveAbilityUtil(AbilitySystemComponent, AbilityToAdd, bActivate);
	}
	else
	{
		GiveAbilityUtil(AbilitySystemComponent, AbilityToAdd, bActivate);
	}
	
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
