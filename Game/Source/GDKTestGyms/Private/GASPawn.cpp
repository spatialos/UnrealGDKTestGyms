// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GASPawn.h"

#include "AbilitySystemComponent.h"

AGASPawn::AGASPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, AbilitySystem(CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem")))
{
}

void AGASPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Ability system is initially set up before the pawn is possessed so it doesn't
	// have the correct information about the controller, which causes issues with
	// remote activated abilities. Refreshing actor info after possession fixes this.
	AbilitySystem->RefreshAbilityActorInfo();

	// Trigger blueprint event so inherited classes know when to set up abilities.
	OnAbilitySystemReady();
}

void AGASPawn::OnRep_Controller()
{
	Super::OnRep_Controller();

	// Refresh ability system after receiving controller so it's set up correctly
	// on the owning client (otherwise won't attempt to activate ability remotely).
	AbilitySystem->RefreshAbilityActorInfo();
}
