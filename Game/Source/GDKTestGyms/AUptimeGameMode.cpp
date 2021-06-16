// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "AUptimeGameMode.h"

#include "Engine/World.h"
#include "Interop/SpatialWorkerFlags.h"

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

void AUptimeGameMode::BindWorkerFlagDelegates(USpatialWorkerFlags* SpatialWorkerFlags)
{
	Super::BindWorkerFlagDelegates(SpatialWorkerFlags);
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnEgressSizeFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(EgressSizeWorkerFlag, WorkerFlagDelegate);
	}
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnEgressFrequencyFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(EgressFrequencyWorkerFlag, WorkerFlagDelegate);
	}
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnCrossServerSizeFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(CrossServerSizeWorkerFlag, WorkerFlagDelegate);
	}
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnCrossServerFrequencyFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(CrossServerFrequencyCommandLineKey, WorkerFlagDelegate);
	}
}

void AUptimeGameMode::ReadCommandLineArgs(const FString& CommandLine)
{
	Super::ReadCommandLineArgs(CommandLine);
	FParse::Value(*CommandLine, *EgressSizeCommandLineKey, EgressSize);
	FParse::Value(*CommandLine, *EgressFrequencyCommandLineKey, EgressFrequency);
	FParse::Value(*CommandLine, *CrossServerSizeCommandLineKey, CrossServerSize);
	FParse::Value(*CommandLine, *CrossServerFrequencyCommandLineKey, CrossServerFrequency);

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("EgressSize %d, EgressFrequency %d, CrossServerSize %d, CrossServerFrequency %d"), EgressSize, EgressFrequency, CrossServerSize, CrossServerFrequency);
}

void AUptimeGameMode::ReadWorkerFlagValues(USpatialWorkerFlags* SpatialWorkerFlags)
{
	Super::ReadWorkerFlagValues(SpatialWorkerFlags);
	FString EgressSizeString, EgressFrequencyString, CrossServerSizeString, CrossServerFrequencyString;

	if (SpatialWorkerFlags->GetWorkerFlag(EgressSizeWorkerFlag, EgressSizeString))
	{
		EgressSize = FCString::Atoi(*EgressSizeString);
	}

	if (SpatialWorkerFlags->GetWorkerFlag(EgressFrequencyWorkerFlag, EgressFrequencyString))
	{
		EgressFrequency = FCString::Atoi(*EgressFrequencyString);
	}

	if (SpatialWorkerFlags->GetWorkerFlag(CrossServerSizeWorkerFlag, CrossServerSizeString))
	{
		CrossServerSize = FCString::Atoi(*CrossServerSizeString);
	}

	if (SpatialWorkerFlags->GetWorkerFlag(CrossServerFrequencyCommandLineKey, CrossServerFrequencyString))
	{
		CrossServerFrequency = FCString::Atoi(*CrossServerFrequencyString);
	}

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("EgressSize %d, EgressFrequency %d, CrossServerSize %d, CrossServerFrequency %d"), EgressSize, EgressFrequency, CrossServerSize, CrossServerFrequency);
}

void AUptimeGameMode::StartCustomNPCSpawning()
{
	Super::StartCustomNPCSpawning();
	SpawnCrossServerActors(GetNumWorkers());
}

void AUptimeGameMode::SpawnCrossServerActors(int32 CrossServerPointNum)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Error spawning, World is null"));
		return;
	}

	TArray<FVector> Locations = GenerateCrossServerLoaction();
	auto SizeOfLocations = Locations.Num();
	for (auto i = 0; i < SizeOfLocations; ++i)
	{
		AUptimeCrossServerBeacon* Beacon = World->SpawnActor<AUptimeCrossServerBeacon>(CrossServerClass, Locations[i], FRotator::ZeroRotator, FActorSpawnParameters());
		checkf(Beacon, TEXT("Beacon failed to spawn at %s"), *Locations[i].ToString());

		Beacon->SetCrossServerSize(CrossServerSize);
		Beacon->SetCrossServerFrequency(CrossServerFrequency);
		UE_LOG(LogUptimeGymGameMode, Log, TEXT("cross server size: %d, cross server frequency: %d"), CrossServerSize, CrossServerFrequency);
	}
}

TArray<FVector> AUptimeGameMode::GenerateCrossServerLoaction()
{
	const float Height = GetZoneHeight();
	const float Width = GetZoneWidth();
	const float Rows = GetZoningRows();
	const float Cols = GetZoningCols();

	const float DistBetweenRows = Height / Rows;
	const float DistBetweenCols = Width / Cols;

	const float StartingX = -(Cols - 1) * Width / 2 / Cols;
	const float StartingY = -(Rows - 1) * Height / 2 / Rows;

	TArray<FVector> Locations;
	const float Z = 300.0f;
	for (int32 Col = 0; Col < Cols; ++Col)
	{
		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const float X = StartingX + Col * DistBetweenCols;
			const float Y = StartingY + Row * DistBetweenRows;
			FVector Location = FVector(X, Y, Z);
			Locations.Add(Location);
		}
	}
	return Locations;
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