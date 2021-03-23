// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"

#include "GASPawn.generated.h"

class UAbilitySystemComponent;

/*
 * A simple pawn with an ability system component that is set up to handle remote activation of abilities.
 */
UCLASS()
class GDKTESTGYMS_API AGASPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
	AGASPawn(const FObjectInitializer& ObjectInitializer);

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnAbilitySystemReady();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAbilitySystemComponent* AbilitySystem;
};
