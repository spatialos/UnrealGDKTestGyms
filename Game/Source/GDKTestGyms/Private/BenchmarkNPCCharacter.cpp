// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"

DEFINE_LOG_CATEGORY(LogBenchmarkNPCCharacter);

void ABenchmarkNPCCharacter::OnAuthorityGained()
{
	Super::OnAuthorityGained();
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("OnAuthorityGained"));
}

void ABenchmarkNPCCharacter::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	if (Controller)
	{
		Controller->Destroy();
	}
}


