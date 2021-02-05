// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GasCharacter.h"
#include "AbilityGivingGymCharacter.generated.h"

/**
 *
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
