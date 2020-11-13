// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GasCube.generated.h"

/**
* A GDK Test Gyms Character with an Ability System Component added and set up to correctly initialise.
*/
UCLASS()
class GDKTESTGYMS_API AGasCube : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

	// Implement IAbilitySystemInterface.
	UAbilitySystemComponent* GetAbilitySystemComponent() const override	
	{
		return AbilitySystemComponent;
	}

public:
	AGasCube();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	UAbilitySystemComponent* AbilitySystemComponent;
};
