// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GASTestActorBase.generated.h"

/**
 * A replicated Actor with a Cube Mesh, used as a base for Actors used in spatial tests.
 */
UCLASS()
class AGASTestActorBase : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Implement IAbilitySystemInterface.
	UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	AGASTestActorBase();

	virtual void BeginPlay() override;
	virtual void OnAuthorityGained() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	UAbilitySystemComponent* AbilitySystemComponent;

private:
	virtual TArray<TSubclassOf<UGameplayAbility>> GetInitialGrantedAbilities() { return {}; }

	void GrantInitialAbilitiesIfNeeded();

	UPROPERTY(Handover)
	bool bHasGrantedAbilities;
};
