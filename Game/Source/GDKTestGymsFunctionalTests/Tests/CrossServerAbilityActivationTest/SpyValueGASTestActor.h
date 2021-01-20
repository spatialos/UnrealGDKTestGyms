// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GDKTestGymsFunctionalTests/Tests/TestActors/GASTestActorBase.h"
#include "SpyValueGASTestActor.generated.h"

/**
 * An actor with a replicated counter. This allows an auth server to increment the counter when an action was taken, and a non-auth server
 * to verify that the action took place
 */
UCLASS()
class ASpyValueGASTestActor : public AGASTestActorBase
{
	GENERATED_BODY()

public:
	ASpyValueGASTestActor();

	void IncrementCounter();
	int GetCounter() const;

	UFUNCTION(CrossServer, Reliable)
	void ResetCounter();

private:
	virtual TArray<TSubclassOf<UGameplayAbility>> GetInitialGrantedAbilities() override;

	UPROPERTY(Replicated)
	int Counter;
};
