// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"


DEFINE_LOG_CATEGORY(LogBenchmarkNPCCharacter);

void ABenchmarkNPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("BeginPlay %s"), *GetName());
}

void ABenchmarkNPCCharacter::OnAuthorityGained()
{
	AController* ControllerS = GetController();
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("OnAuthorityGained %S - %s"), *GetName(), ControllerS ? *ControllerS->GetName(): TEXT("NOPE"));
}

void ABenchmarkNPCCharacter::SpawnDefaultController()
{
	if (Controller != nullptr || GetNetMode() == NM_Client)
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController NM_Client %S"), *GetName());
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
			UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController Possess %S"), *GetName());
			NewController->Possess(this);
		}
		else
		{
			UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController NewController == nullptr %S"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController AIControllerClass == nullptr %S"), *GetName());
	}
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("SpawnDefaultController %S"), *GetName());
}

void ABenchmarkNPCCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UE_LOG(LogBenchmarkNPCCharacter, Log, TEXT("PossessedBy %S"), *GetName());
}

void ABenchmarkNPCCharacter::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	if (Controller)
	{
		Controller->Destroy();
	}
}


