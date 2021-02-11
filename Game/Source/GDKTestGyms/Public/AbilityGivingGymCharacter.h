// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GasCharacter.h"
#include "AbilityGivingGymCharacter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAbilityGivingCharacter, Log, All);

/**
 * Character with keybindings on Q and E, which give an ability at level 1 or 2 respectively to the character on the server.
 */
UCLASS()
class GDKTESTGYMS_API AAbilityGivingGymCharacter : public AGasCharacter
{
	GENERATED_BODY()

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void GiveAbilityOne() { ServerGiveAbility(1); }
	void GiveAbilityTwo() { ServerGiveAbility(2); }

	UFUNCTION(Server, Reliable)
	void ServerGiveAbility(int32 Level);

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> AbilityToAdd; // To be set in the blueprint child of this class
};
