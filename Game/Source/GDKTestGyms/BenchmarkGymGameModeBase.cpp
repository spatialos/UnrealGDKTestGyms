// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameModeBase.h"

#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GDKTestGymsGameInstance.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Misc/CommandLine.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameModeBase);

const FString ABenchmarkGymGameModeBase::TotalPlayerWorkerFlag = TEXT("total_players");
const FString ABenchmarkGymGameModeBase::TotalNPCsWorkerFlag = TEXT("total_npcs");
const FString ABenchmarkGymGameModeBase::TotalPlayerCommandLineArg = TEXT("TotalPlayers");
const FString ABenchmarkGymGameModeBase::TotalNPCsCommandLineArg = TEXT("TotalNPCs");

// Metrics
namespace
{
	const FString AverageClientRTTMetricName = TEXT("UnrealAverageClientRTT");
	const FString AverageClientViewLatenessMetricName = TEXT("UnrealAverageClientViewLateness");
	const FString PlayersSpawnedMetricName = TEXT("UnrealActivePlayers");
} // anonymous namespace

ABenchmarkGymGameModeBase::ABenchmarkGymGameModeBase()
	: ExpectedPlayers(1)
	, PrintUXMetric(10.0f)
	, MaxClientRoundTripSeconds(150)
	, MaxClientViewLatenessSeconds(150)
	, bPlayersHaveJoined(false)
	, bHasUxFailed(false)
	, bHasFpsFailed(false)
	// These values need to match the GDK scenario validation equivalents
	, MinAcceptableFPS(20.0f) // Same for both client and server currently
	, MinDelayFPS(120.0f)
	, ActivePlayers(0)
{
	PrimaryActorTick.bCanEverTick = true;
	SecondsTillPlayerCheck = 15.0f * 60.0f;

	// Seamless Travel is not currently supported in SpatialOS [UNR-897]
	bUseSeamlessTravel = false;
}

void ABenchmarkGymGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	ParsePassedValues();

	USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
	USpatialMetrics* SpatialMetrics = SpatialDriver != nullptr ? SpatialDriver->SpatialMetrics : nullptr;
	if (SpatialMetrics != nullptr)
	{
		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientRTT);
			SpatialMetrics->SetCustomMetric(AverageClientRTTMetricName, Delegate);
		}

		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientViewLateness);
			SpatialMetrics->SetCustomMetric(AverageClientViewLatenessMetricName, Delegate);
		}

		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetPlayersConnected);
			SpatialMetrics->SetCustomMetric(PlayersSpawnedMetricName, Delegate);
		}
	}
}

void ABenchmarkGymGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickFPSCheck(DeltaSeconds);
	TickPlayersConnectedCheck(DeltaSeconds);
	TickUXMetricCheck(DeltaSeconds);
}

void ABenchmarkGymGameModeBase::TickPlayersConnectedCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	if (SecondsTillPlayerCheck > 0.0f)
	{
		SecondsTillPlayerCheck -= DeltaSeconds;
		if (SecondsTillPlayerCheck <= 0.0f)
		{
			if (ActivePlayers != ExpectedPlayers)
			{
				// This log is used by the NFR pipeline to indicate if a client failed to connect
				UE_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("A client connection was dropped. Expected %d, got %d"), ExpectedPlayers, GetNumPlayers());
			}
			else
			{
				// Useful for NFR log inspection
				UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("All clients successfully connected. Expected %d, got %d"), ExpectedPlayers, GetNumPlayers());
			}
		}
	}
}

void ABenchmarkGymGameModeBase::TickFPSCheck(float DeltaSeconds)
{
	if (MinDelayFPS > 0.0f)
	{
		MinDelayFPS -= DeltaSeconds;
	}

	if (MinDelayFPS <= 0.0f && !bHasFpsFailed && GetWorld() != nullptr)
	{
		if (const UGDKTestGymsGameInstance* GameInstance = Cast<UGDKTestGymsGameInstance>(GetWorld()->GetGameInstance()))
		{
			float FPS = GameInstance->GetAveragedFPS();
			if (FPS < MinAcceptableFPS)
			{
				bHasFpsFailed = true;
#if !WITH_EDITOR
				UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("FPS check failed. FPS: %.8f"), FPS);
#endif
			}
		}
	}
}

void ABenchmarkGymGameModeBase::TickUXMetricCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	float ClientRTTSeconds = 0.0f;
	int UXComponentCount = 0;
	float ClientViewLatenessSeconds = 0.0f;
	for (TObjectIterator<UUserExperienceReporter> Itr; Itr; ++Itr) // These exist on player characters
	{
		UUserExperienceReporter* Component = *Itr;
		if (Component->GetOwner() != nullptr && Component->GetWorld() == GetWorld())
		{
			ClientRTTSeconds += Component->ServerRTT;
			UXComponentCount++;

			ClientViewLatenessSeconds += Component->ServerViewLateness;
		}
	}

	ActivePlayers = UXComponentCount;

	if (UXComponentCount > 0)
	{
		bPlayersHaveJoined = true;
	}

	if (!bPlayersHaveJoined)
	{
		return; // We don't start reporting until there are some valid components in the scene.
	}

	ClientRTTSeconds /= static_cast<float>(UXComponentCount) + 0.00001f; // Avoid div 0
	ClientViewLatenessSeconds /= static_cast<float>(UXComponentCount) + 0.00001f; // Avoid div 0

	AveragedClientRTTSeconds = ClientRTTSeconds;
	AveragedClientViewLatenessSeconds = ClientViewLatenessSeconds;

	if (!bHasUxFailed && AveragedClientRTTSeconds > MaxClientRoundTripSeconds && AveragedClientViewLatenessSeconds > MaxClientViewLatenessSeconds)
	{
		bHasUxFailed = true;
#if !WITH_EDITOR
		UE_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("UX metric has failed. RTT: %.8f, ViewLateness: %.8f, ActivePlayers: %d"), AveragedClientRTTSeconds, AveragedClientViewLatenessSeconds, ActivePlayers);
#endif
	}

	PrintUXMetric -= DeltaSeconds;
	if (PrintUXMetric < 0.0f)
	{
		PrintUXMetric = 10.0f;
#if !WITH_EDITOR
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("UX metric values. RTT: %.8f, ViewLateness: %.8f, ActivePlayers: %d"), AveragedClientRTTSeconds, AveragedClientViewLatenessSeconds, ActivePlayers);
#endif
	}
}

void ABenchmarkGymGameModeBase::ParsePassedValues()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("TotalPlayers")))
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Found OverrideSpawning in command line args, worker flags for custom spawning will be ignored."));
		FParse::Value(FCommandLine::Get(), *TotalPlayerCommandLineArg, ExpectedPlayers);
		FParse::Value(FCommandLine::Get(), *TotalNPCsCommandLineArg, TotalNPCs);
		FParse::Value(FCommandLine::Get(), TEXT("MaxRoundTrip="), MaxClientRoundTripSeconds);
		FParse::Value(FCommandLine::Get(), TEXT("MaxLateness="), MaxClientViewLatenessSeconds);
	}
	else
	{
		USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver());
		check(NetDriver);

		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString TotalPlayersString, TotalNPCsString, MaxRoundTrip, MaxViewLateness;

		const USpatialWorkerFlags* SpatialWorkerFlags = NetDriver != nullptr ? NetDriver->SpatialWorkerFlags : nullptr;
		if (SpatialWorkerFlags != nullptr)
		{
			if (SpatialWorkerFlags->GetWorkerFlag(TotalPlayerWorkerFlag, TotalPlayersString))
			{
				ExpectedPlayers = FCString::Atoi(*TotalPlayersString);
			}

			if (NetDriver->SpatialWorkerFlags->GetWorkerFlag(TotalNPCsWorkerFlag, TotalNPCsString))
			{
				TotalNPCs = FCString::Atoi(*TotalNPCsString);
			}

			if (NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("max_round_trip"), MaxRoundTrip))
			{
				MaxClientRoundTripSeconds = FCString::Atoi(*MaxRoundTrip);
			}

			if (NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("max_lateness"), MaxViewLateness))
			{
				MaxClientViewLatenessSeconds = FCString::Atoi(*MaxViewLateness);
			}
		}
	}

	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Players %d, NPCs %d"), ExpectedPlayers, TotalNPCs);
}