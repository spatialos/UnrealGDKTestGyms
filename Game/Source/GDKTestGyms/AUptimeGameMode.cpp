// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "AUptimeGameMode.h"

#include "Engine/World.h"
//#include "Interop/SpatialWorkerFlags.h"

DEFINE_LOG_CATEGORY(LogUptimeGymGameMode);

namespace
{
	const FString EgressSizeWorkerFlag = TEXT("egress_test_size");
	const FString EgressSizeCommandLineKey = TEXT("-EgressTestSize=");
	
	const FString EgressFrequencyWorkerFlag = TEXT("egress_test_frequency");
	const FString EgressFrequencyCommandLineKey = TEXT("-EgressTestFrequency=");
	
	const FString CrossServerSizeWorkerFlag = TEXT("cross_server_size");
	const FString CrossServerSizeCommandLineKey = TEXT("-CrossServerSize=");

	const FString CrossServerFrequencyWorkerFlag = TEXT("cross_server_frequency");
	const FString CrossServerFrequencyCommandLineKey = TEXT("-CrossServerFrequency=");
} // anonymous namespace

AUptimeGameMode::AUptimeGameMode()
	: EgressSize(0)
	, EgressFrequency(0)
	, CrossServerSize(0)
	, CrossServerFrequency(0)
{
	PercentageSpawnPointsOnWorkerBoundaries = 0.0f;
	PrimaryActorTick.bCanEverTick = true;
}

// void AUptimeGameMode::BindWorkerFlagDelegates(USpatialWorkerFlags* SpatialWorkerFlags)
// {
// 	Super::BindWorkerFlagDelegates(SpatialWorkerFlags);
//  	{
//  		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
//  		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnEgressSizeFlagUpdate);
//  		SpatialWorkerFlags->RegisterFlagUpdatedCallback(EgressSizeWorkerFlag, WorkerFlagDelegate);
//  	}
//  	{
//  		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
//  		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnEgressFrequencyFlagUpdate);
//  		SpatialWorkerFlags->RegisterFlagUpdatedCallback(EgressFrequencyWorkerFlag, WorkerFlagDelegate);
//  	}
//  	{
//  		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
//  		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnCrossServerSizeFlagUpdate);
//  		SpatialWorkerFlags->RegisterFlagUpdatedCallback(CrossServerSizeWorkerFlag, WorkerFlagDelegate);
//  	}
//  	{
//  		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
//  		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnCrossServerFrequencyFlagUpdate);
//  		SpatialWorkerFlags->RegisterFlagUpdatedCallback(CrossServerFrequencyCommandLineKey, WorkerFlagDelegate);
//  	}
// }

void AUptimeGameMode::ReadCommandLineArgs(const FString& CommandLine)
{
	Super::ReadCommandLineArgs(CommandLine);
	FParse::Value(*CommandLine, *EgressSizeCommandLineKey, EgressSize);
	FParse::Value(*CommandLine, *EgressFrequencyCommandLineKey, EgressFrequency);
	FParse::Value(*CommandLine, *CrossServerSizeCommandLineKey, CrossServerSize);
	FParse::Value(*CommandLine, *CrossServerFrequencyCommandLineKey, CrossServerFrequency);

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("EgressSize %d, EgressFrequency %d, CrossServerSize %d, CrossServerFrequency %d"), EgressSize, EgressFrequency, CrossServerSize, CrossServerFrequency);
}

// void AUptimeGameMode::ReadWorkerFlagValues(USpatialWorkerFlags* SpatialWorkerFlags)
// {
// 	Super::ReadWorkerFlagValues(SpatialWorkerFlags);
// 	FString EgressSizeString, EgressFrequencyString, CrossServerSizeString, CrossServerFrequencyString;
// 
// 	if (SpatialWorkerFlags->GetWorkerFlag(EgressSizeWorkerFlag, EgressSizeString))
// 	{
// 		EgressSize = FCString::Atoi(*EgressSizeString);
// 	}
// 
// 	if (SpatialWorkerFlags->GetWorkerFlag(EgressFrequencyWorkerFlag, EgressFrequencyString))
// 	{
// 		EgressFrequency = FCString::Atoi(*EgressFrequencyString);
// 	}
// 
// 	if (SpatialWorkerFlags->GetWorkerFlag(CrossServerSizeWorkerFlag, CrossServerSizeString))
// 	{
// 		CrossServerSize = FCString::Atoi(*CrossServerSizeString);
// 	}
// 
// 	if (SpatialWorkerFlags->GetWorkerFlag(CrossServerFrequencyCommandLineKey, CrossServerFrequencyString))
// 	{
// 		CrossServerFrequency = FCString::Atoi(*CrossServerFrequencyString);
// 	}
// 
// 	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("EgressSize %d, EgressFrequency %d, CrossServerSize %d, CrossServerFrequency %d"), EgressSize, EgressFrequency, CrossServerSize, CrossServerFrequency);
// }

void AUptimeGameMode::StartCustomNPCSpawning()
{
	Super::StartCustomNPCSpawning();
	SpawnCrossServerActors();
}

void AUptimeGameMode::SpawnCrossServerActors()
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Error spawning, World is null"));
		return;
	}

	if (SpawnManager == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Error spawning, SpawnManager is null"));
		return;
	}

	SpawnManager->ForEachArea([this, World](FSpawnArea& ZoneArea) {
		AActor* SpawnPointActor = ZoneArea.GetSpawnPointActorByIndex(0);
		if (SpawnPointActor == nullptr)
		{
			return;
		}

		FVector SpawnLocation = SpawnPointActor->GetActorLocation();

		AUptimeCrossServerBeacon* Beacon = World->SpawnActor<AUptimeCrossServerBeacon>(CrossServerClass, SpawnLocation, FRotator::ZeroRotator, FActorSpawnParameters());
		checkf(Beacon, TEXT("Beacon failed to spawn at %s"), *SpawnLocation.ToString());

		Beacon->SetCrossServerSize(CrossServerSize);
		Beacon->SetCrossServerFrequency(CrossServerFrequency);
		UE_LOG(LogUptimeGymGameMode, Log, TEXT("cross server size: %d, cross server frequency: %d"), CrossServerSize, CrossServerFrequency);
	}, FSpawnArea::AreaType::Zone);
}

void AUptimeGameMode::OnEgressSizeFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	EgressSize = FCString::Atoi(*FlagValue);
	UE_LOG(LogUptimeGymGameMode, Log, TEXT("EgressSize %d"), EgressSize);
}

void AUptimeGameMode::OnEgressFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	EgressFrequency = FCString::Atoi(*FlagValue);
	UE_LOG(LogUptimeGymGameMode, Log, TEXT("EgressFrequency %d"), EgressFrequency);
}

void AUptimeGameMode::OnCrossServerSizeFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	CrossServerSize = FCString::Atoi(*FlagValue);
	UE_LOG(LogUptimeGymGameMode, Log, TEXT("CrossServerSize %d"), CrossServerSize);
}

void AUptimeGameMode::OnCrossServerFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	CrossServerFrequency = FCString::Atoi(*FlagValue);
	UE_LOG(LogUptimeGymGameMode, Log, TEXT("CrossServerFrequency %d"), CrossServerFrequency);
}