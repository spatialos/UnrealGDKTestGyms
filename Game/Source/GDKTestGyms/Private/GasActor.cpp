// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GasActor.h"

AGasActor::AGasActor()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}
