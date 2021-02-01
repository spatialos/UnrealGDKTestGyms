// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GasCharacter.h"
#include "SpecHandleTestCharacter.generated.h"

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API ASpecHandleTestCharacter : public AGasCharacter
{
	GENERATED_BODY()

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable)
	void GiveAbilityAndLogActivatableAbilities();
};
