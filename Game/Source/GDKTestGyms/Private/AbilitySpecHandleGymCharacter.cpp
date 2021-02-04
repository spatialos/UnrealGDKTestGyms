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
static void LogCheck(bool bSuccess, FString Message)
{
	UE_LOG(LogTemp, Log, TEXT("%s | %s"), bSuccess ? TEXT("Success") : TEXT("Failure"), *Message);
}

static void VerifyNoDuplicateHandles(UAbilitySystemComponent& AbilitySystemComponent)
{
	const TArray<FGameplayAbilitySpec>& Specs = AbilitySystemComponent.GetActivatableAbilities();

	bool bAllUnique = true;
	for (auto& SpecA : Specs)
	{
		int SameHandleCount = 0;
		for (auto& SpecB : Specs)
		{
			if (SpecA.Handle == SpecB.Handle)
			{
				SameHandleCount++;
			}
		}

		if (SameHandleCount > 1)
		{
			bAllUnique = false;
			break;
		}
	}
	
	LogCheck(bAllUnique, TEXT("All activatable abilities have unqiue handles."));
	
	if (!bAllUnique)
	{
		UE_LOG(LogTemp, Log, TEXT("List of all activatable abilities:"));
		for (auto& Spec : Specs)
		{
			UE_LOG(LogTemp, Log, TEXT("  %s (%s)"), *GetNameSafe(Spec.Ability), *Spec.Handle.ToString());
		}
	}
}


static FGameplayAbilitySpecHandle GiveAbilityUtil(UAbilitySystemComponent& AbilitySystemComponent, TSubclassOf<UGameplayAbility> AbilityToAdd, bool bActivate)
{
	const FGameplayAbilitySpec Spec(AbilityToAdd);	
	FGameplayAbilitySpecHandle Handle;
	if (bActivate)
	{
		Handle = AbilitySystemComponent.GiveAbilityAndActivateOnce(Spec);
	} else
	{
		Handle = AbilitySystemComponent.GiveAbility(Spec);
	}
	return Handle;
}

void AAbilitySpecHandleGymCharacter::ServerGiveAbility_Implementation(bool bActivate, bool bLock)
{
	UE_LOG(LogTemp, Log, TEXT("ServerGiveAbility on %s"), *GetNameSafe(this));
	FGameplayAbilitySpecHandle Handle;
	if (bLock)
	{
		// Lock the ASC ability list for this scope
		// Can't use ABILITYLIST_SCOPE_LOCK since that assumes that `this` is of type UAbilitySystemComponent*
		FScopedAbilityListLock Lock(*AbilitySystemComponent);
		Handle = GiveAbilityUtil(*AbilitySystemComponent, AbilityToAdd, bActivate);

		LogCheck(AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) == nullptr, TEXT("Spec has not been added while the ASC is locked."));
	}
	else
	{
		Handle = GiveAbilityUtil(*AbilitySystemComponent, AbilityToAdd, bActivate);
	}

	LogCheck(AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) != nullptr, TEXT("Spec has been added and can be found via its handle."));

	VerifyNoDuplicateHandles(*AbilitySystemComponent);
}

