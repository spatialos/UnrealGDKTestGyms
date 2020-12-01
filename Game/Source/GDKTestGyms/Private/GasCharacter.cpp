// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GasCharacter.h"

AGasCharacter::AGasCharacter()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}

void AGasCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AGasCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	AbilitySystemComponent->RefreshAbilityActorInfo();
}