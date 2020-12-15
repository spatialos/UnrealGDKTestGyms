// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GasActor.generated.h"

/**
* An actor with an Ability System Component added and exposed via IAbilitySystemInterface. 
*/
UCLASS()
class GDKTESTGYMS_API AGasActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

	// Implement IAbilitySystemInterface.
	UAbilitySystemComponent* GetAbilitySystemComponent() const override	
	{
		return AbilitySystemComponent;
	}

public:
	AGasActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	UAbilitySystemComponent* AbilitySystemComponent;
};
