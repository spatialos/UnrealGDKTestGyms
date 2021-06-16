// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "AUptimeGameMode.h"

#include "Engine/World.h"
#include "Interop/SpatialWorkerFlags.h"

DEFINE_LOG_CATEGORY(LogUptimeGymGameMode);

namespace
{
	const FString UptimeEgressSizeWorkerFlag = TEXT("egress_test_size");
	const FString UptimeEgressSizeCommandLineKey = TEXT("-EgressTestSize=");
	
	const FString UptimeEgressFrequencyWorkerFlag = TEXT("egress_test_frequency");
	const FString UptimeEgressFrequencyCommandLineKey = TEXT("-EgressTestFrequency=");
	
	const FString UptimeCrossServerSizeWorkerFlag = TEXT("cross_server_size");
	const FString UptimeCrossServerSizeCommandLineKey = TEXT("-CrossServerSize=");

	const FString UptimeCrossServerFrequencyWorkerFlag = TEXT("cross_server_frequency");
	const FString UptimeCrossServerFrequencyCommandLineKey = TEXT("-CrossServerFrequency=");
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

void AUptimeGameMode::BindWorkerFlagsDelegates(USpatialWorkerFlags* SpatialWorkerFlags)
{
	Super::BindWorkerFlagsDelegates(SpatialWorkerFlags);
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnUptimeEgressSizeFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(UptimeEgressSizeWorkerFlag, WorkerFlagDelegate);
	}
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnUptimeEgressFrequencyFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(UptimeEgressFrequencyWorkerFlag, WorkerFlagDelegate);
	}
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnUptimeCrossServerSizeFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(UptimeCrossServerSizeWorkerFlag, WorkerFlagDelegate);
	}
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &AUptimeGameMode::OnUptimeCrossServerFrequencyFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(UptimeCrossServerFrequencyCommandLineKey, WorkerFlagDelegate);
	}
}

void AUptimeGameMode::ReadCommandLineArgs(const FString& CommandLine)
{
	Super::ReadCommandLineArgs(CommandLine);
	FParse::Value(*CommandLine, *UptimeEgressSizeCommandLineKey, EgressSize);
	FParse::Value(*CommandLine, *UptimeEgressFrequencyCommandLineKey, EgressFrequency);
	FParse::Value(*CommandLine, *UptimeCrossServerSizeCommandLineKey, CrossServerSize);
	FParse::Value(*CommandLine, *UptimeCrossServerFrequencyCommandLineKey, CrossServerFrequency);
}

void AUptimeGameMode::ReadWorkerFlagsValues(USpatialWorkerFlags* SpatialWorkerFlags)
{
	Super::ReadWorkerFlagsValues(SpatialWorkerFlags);
	FString EgressSizeString, EgressFrequencyString, CrossServerSizeString, CrossServerFrequencyString;

	if (SpatialWorkerFlags->GetWorkerFlag(UptimeEgressSizeWorkerFlag, EgressSizeString))
	{
		EgressSize = FCString::Atoi(*EgressSizeString);
	}

	if (SpatialWorkerFlags->GetWorkerFlag(UptimeEgressFrequencyWorkerFlag, EgressFrequencyString))
	{
		EgressFrequency = FCString::Atoi(*EgressFrequencyString);
	}

	if (SpatialWorkerFlags->GetWorkerFlag(UptimeCrossServerSizeWorkerFlag, CrossServerSizeString))
	{
		CrossServerSize = FCString::Atoi(*CrossServerSizeString);
	}

	if (SpatialWorkerFlags->GetWorkerFlag(UptimeCrossServerFrequencyCommandLineKey, CrossServerFrequencyString))
	{
		CrossServerFrequency = FCString::Atoi(*CrossServerFrequencyString);
	}
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
	const int32 Z = 300;
	for (int32 x = 0; x < Cols; ++x)
	{
		for (int32 y = 0; y < Rows; ++y)
		{
			const float X = StartingX + x * DistBetweenCols;
			const float Y = StartingY + y * DistBetweenRows;
			FVector Location = FVector(X, Y, Z);
			Locations.Add(Location);
		}
	}
	return Locations;
}

void AUptimeGameMode::OnUptimeEgressSizeFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	EgressSize = FCString::Atoi(*FlagValue);
}

void AUptimeGameMode::OnUptimeEgressFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	EgressFrequency = FCString::Atoi(*FlagValue);
}

void AUptimeGameMode::OnUptimeCrossServerSizeFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	CrossServerSize = FCString::Atoi(*FlagValue);
}

void AUptimeGameMode::OnUptimeCrossServerFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	CrossServerFrequency = FCString::Atoi(*FlagValue);
}