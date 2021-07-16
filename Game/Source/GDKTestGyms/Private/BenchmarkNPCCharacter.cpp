// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"
#include "DeterministicBlackboardValues.h"

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
		SpawnDefaultController();
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController"));
		UDeterministicBlackboardValues* Component = FindComponentByClass<UDeterministicBlackboardValues>();
		if (Component != nullptr)
		{
			Component->ApplyBlackboardValues();
		}
	}

	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("OnAuthorityGained %s - %s"), *GetName(), Controller ? *Controller->GetName(): TEXT("NOPE"));
}

void ABenchmarkNPCCharacter::SpawnDefaultController()
{
	if (Controller != nullptr || GetNetMode() == NM_Client)
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController NM_Client %s"), *GetName());
		return;
	}

	if (AIControllerClass != nullptr)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Instigator = GetInstigator();
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.OverrideLevel = GetLevel();
		SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save AI controllers into a map
		AController* NewController = GetWorld()->SpawnActor<AController>(AIControllerClass, GetActorLocation(), GetActorRotation(), SpawnInfo);
		if (NewController != nullptr)
		{
			// if successful will result in setting this->Controller 
			// as part of possession mechanics
			UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController Possess %s"), *GetName());
			NewController->Possess(this);
		}
		else
		{
			UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController NewController == nullptr %s"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController AIControllerClass == nullptr %s"), *GetName());
	}
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController %s"), *GetName());
}

void ABenchmarkNPCCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("PossessedBy %s"), *GetName());
}

void ABenchmarkNPCCharacter::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	if (Controller)
	{
		Controller->Destroy();
	}
}


