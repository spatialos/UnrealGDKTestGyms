// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GDKTestGyms/GDKTestGymsCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GasCharacter.generated.h"

/**
 * A GDK Test Gyms Character with an Ability System Component added and set up to correctly initialise.
 */
UCLASS()
class GDKTESTGYMS_API AGasCharacter : public AGDKTestGymsCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
	//Implement IAbilitySystemInterface.
	UAbilitySystemComponent* GetAbilitySystemComponent() const override	
	{
		return AbilitySystemComponent;
	}

public:
	AGasCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UAbilitySystemComponent* AbilitySystemComponent;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
};
