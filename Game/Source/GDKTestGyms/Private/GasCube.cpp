// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GasCube.h"

AGasCube::AGasCube()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}