// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

	UFUNCTION(Server, Reliable)
	void GiveAbility();

	UFUNCTION(Server, Reliable)
    void GiveAbilityWhileLocked();

	void LogActivatableAbilities() const;
};
