// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GasCharacter.h"
#include "AbilitySpecHandleGymCharacter.generated.h"

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API AAbilitySpecHandleGymCharacter : public AGasCharacter
{
	GENERATED_BODY()

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void GiveAbility() { ServerGiveAbility(false, false); }
    void GiveAbilityWhileLocked() { ServerGiveAbility(false, true); }

    void GiveAbilityAndActivateOnce() { ServerGiveAbility(true, false); }
    void GiveAbilityAndActivateOnceWhileLocked() { ServerGiveAbility(true, true); }

	UFUNCTION(Server, Reliable)
	void ServerGiveAbility(bool bActivate, bool bLock);

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> AbilityToAdd; // To be set in the blueprint child of this class
};
