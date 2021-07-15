// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"


DEFINE_LOG_CATEGORY(LogBenchmarkNPCCharacter);

void ABenchmarkNPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("BeginPlay"));
}

void ABenchmarkNPCCharacter::OnAuthorityGained()
{
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


