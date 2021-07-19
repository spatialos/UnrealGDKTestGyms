// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"

DEFINE_LOG_CATEGORY(LogBenchmarkNPCCharacter);

void ABenchmarkNPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("BeginPlay %s"), *GetName());
}

void ABenchmarkNPCCharacter::Tick(float Delta)
{
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("Tick %s - %s"), *GetName(), Controller ? *Controller->GetName() : TEXT("NOPE"));
}

void ABenchmarkNPCCharacter::OnAuthorityGained()
{
	Super::OnAuthorityGained();
	if (Controller == nullptr)
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController"));
	}

	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("OnAuthorityGained %s - %s"), *GetName(), Controller ? *Controller->GetName() : TEXT("NOPE"));
}

void ABenchmarkNPCCharacter::SpawnDefaultController()
{
	if (Controller != nullptr || GetNetMode() == NM_Client)
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController NM_Client %s"), *GetName());
	}

	if (AIControllerClass == nullptr)
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController AIControllerClass == nullptr %s"), *GetName());
	}
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController %s"), *GetName());

	Super::SpawnDefaultController();

}

void ABenchmarkNPCCharacter::PossessedBy(AController* NewController)
{
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("PossessedBy %s"), *GetName());
	Super::PossessedBy(NewController);
}

void ABenchmarkNPCCharacter::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	if (Controller)
	{
		Controller->Destroy();
	}
}


