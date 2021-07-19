// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"

DEFINE_LOG_CATEGORY(LogBenchmarkNPCCharacter);

void ABenchmarkNPCCharacter::OnAuthorityGained()
{
	Super::OnAuthorityGained();
	if (Controller == nullptr)
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController"));
	}

	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("OnAuthorityGained %s - %s"), *GetName(), Controller ? *Controller->GetName() : TEXT("NOPE"));
}

void ABenchmarkNPCCharacter::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	if (Controller)
	{
		Controller->Destroy();
	}
}
