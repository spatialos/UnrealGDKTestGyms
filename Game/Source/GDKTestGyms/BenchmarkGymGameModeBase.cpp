// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameModeBase.h"

#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GDKTestGymsGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Misc/CommandLine.h"
#include "Net/UnrealNetwork.h"
#include "Utils/SpatialMetrics.h"
#include "GDKTestGymsGameInstance.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameModeBase);

// Metrics
namespace
{
	const FString AverageClientRTTMetricName = TEXT("UnrealAverageClientRTT");
	const FString AverageClientViewLatenessMetricName = TEXT("UnrealAverageClientViewLateness");
	const FString PlayersSpawnedMetricName = TEXT("UnrealActivePlayers");
	const FString AverageFPSValid = TEXT("UnrealServerFPSValid");
	const FString AverageClientFPSValid = TEXT("UnrealClientFPSValid");

	const FString MaxRoundTripWorkerFlag = TEXT("max_round_trip");
	const FString MaxLatenessWorkerFlag = TEXT("max_lateness");
	const FString MaxRoundTripCommandLineKey = TEXT("-MaxRoundTrip=");
	const FString MaxLatenessCommandLineKey = TEXT("-MaxLateness=");

	const FString TotalPlayerWorkerFlag = TEXT("total_players");
	const FString TotalNPCsWorkerFlag = TEXT("total_npcs");
	const FString TotalPlayerCommandLineKey = TEXT("-TotalPlayers=");
	const FString TotalNPCsCommandLineKey = TEXT("-TotalNPCs=");

} // anonymous namespace

FString ABenchmarkGymGameModeBase::ReadFromCommandLineKey = TEXT("ReadFromCommandLine");

ABenchmarkGymGameModeBase::ABenchmarkGymGameModeBase()
	: ExpectedPlayers(1)
	, SecondsTillPlayerCheck(15.0f * 60.0f)
	, PrintUXMetric(10.0f)
	, MaxClientRoundTripSeconds(150)
	, MaxClientViewLatenessSeconds(150)
	, bPlayersHaveJoined(false)
	, bHasUxFailed(false)
	, bHasFpsFailed(false)
	, bHasClientFpsFailed(false)
	// These values need to match the GDK scenario validation equivalents
	, MinAcceptableFPS(20.0f) // Same for both client and server currently
	, MinDelayFPS(120.0f)
	, ActivePlayers(0)
{
	SetReplicates(true);
	PrimaryActorTick.bCanEverTick = true;
}

void ABenchmarkGymGameModeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABenchmarkGymGameModeBase, TotalNPCs);
}

void ABenchmarkGymGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	ParsePassedValues();

	USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
	USpatialMetrics* SpatialMetrics = SpatialDriver != nullptr ? SpatialDriver->SpatialMetrics : nullptr;
	if (SpatialMetrics != nullptr)
	{
		if(HasAuthority())
		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientRTT);
			SpatialDriver->SpatialMetrics->SetCustomMetric(AverageClientRTTMetricName, Delegate);
		}

		if (HasAuthority())
		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientViewLateness);
			SpatialDriver->SpatialMetrics->SetCustomMetric(AverageClientViewLatenessMetricName, Delegate);
		}

		if (HasAuthority())
		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetPlayersConnected);
			SpatialDriver->SpatialMetrics->SetCustomMetric(PlayersSpawnedMetricName, Delegate);
		}

		if (HasAuthority())
		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientFPSValid);
			SpatialDriver->SpatialMetrics->SetCustomMetric(AverageClientFPSValid, Delegate);
		}

		// Valid on all workers
		{
			UserSuppliedMetric Delegate;
			Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetFPSValid);
			SpatialDriver->SpatialMetrics->SetCustomMetric(AverageFPSValid, Delegate);
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

	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);
	if (Constants->SamplesForFPSValid() && !bHasFpsFailed && GetWorld() != nullptr)
	{
		if (const UGDKTestGymsGameInstance* GameInstance = Cast<UGDKTestGymsGameInstance>(GetWorld()->GetGameInstance()))
		{
			float FPS = GameInstance->GetAveragedFPS();
			if (FPS < Constants->GetMinServerFPS())
			{
				bHasFpsFailed = true;
#if !WITH_EDITOR 
				UE_LOG(LogBenchmarkGym, Log, TEXT("FPS check failed. FPS: %.8f"), FPS);
#endif		
			}
		}
	}

	bool bClientFpsWasValid = true;
	for (TObjectIterator<UUserExperienceReporter> Itr; Itr; ++Itr) // These exist on player characters
	{
		UUserExperienceReporter* Component = *Itr;
		if (Component->GetOwner() != nullptr && Component->GetWorld() == GetWorld())
		{
			bClientFpsWasValid = bClientFpsWasValid && Component->bFrameRateValid; // Frame rate wait period is performed by the client and returned valid until then
		}
	}

	if (!bHasClientFpsFailed && !bClientFpsWasValid)
	{
		bHasClientFpsFailed = true;
#if !WITH_EDITOR 
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("FPS check failed. A client has failed."));
#endif		
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
	const FString& CommandLine = FCommandLine::Get();
	if (FParse::Param(*CommandLine, *ReadFromCommandLineKey))
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Found ReadFromCommandLineKey in command line Keys, worker flags for custom spawning will be ignored."));

		FParse::Value(*CommandLine, *TotalPlayerCommandLineKey, ExpectedPlayers);

		int32 NumNPCs = 0;
		FParse::Value(*CommandLine, *TotalNPCsCommandLineKey, NumNPCs);
		SetTotalNPCs(NumNPCs);

		FParse::Value(*CommandLine, *MaxRoundTripCommandLineKey, MaxClientRoundTripSeconds);
		FParse::Value(*CommandLine, *MaxLatenessCommandLineKey, MaxClientViewLatenessSeconds);
	}
	else if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver());
		check(NetDriver);

		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString ExpectedPlayersString, TotalNPCsString, MaxRoundTrip, MaxViewLateness;

		USpatialWorkerFlags* SpatialWorkerFlags = NetDriver != nullptr ? NetDriver->SpatialWorkerFlags : nullptr;
		check(SpatialWorkerFlags != nullptr);

		if (SpatialWorkerFlags != nullptr)
		{
			if (SpatialWorkerFlags->GetWorkerFlag(TotalPlayerWorkerFlag, ExpectedPlayersString))
			{
				ExpectedPlayers = FCString::Atoi(*ExpectedPlayersString);
			}

			if (SpatialWorkerFlags->GetWorkerFlag(TotalNPCsWorkerFlag, TotalNPCsString))
			{
				SetTotalNPCs(FCString::Atoi(*TotalNPCsString));
			}

			if (SpatialWorkerFlags->GetWorkerFlag(MaxRoundTripWorkerFlag, MaxRoundTrip))
			{
				MaxClientRoundTripSeconds = FCString::Atoi(*MaxRoundTrip);
			}

			if (SpatialWorkerFlags->GetWorkerFlag(MaxLatenessWorkerFlag, MaxViewLateness))
			{
				MaxClientViewLatenessSeconds = FCString::Atoi(*MaxViewLateness);
			}

			FOnWorkerFlagsUpdatedBP WorkerFlagDelegate;
			WorkerFlagDelegate.BindDynamic(this, &ABenchmarkGymGameModeBase::OnWorkerFlagUpdated);
			SpatialWorkerFlags->BindToOnWorkerFlagsUpdated(WorkerFlagDelegate);
		}
	}

	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Players %d, NPCs %d, RoundTrip %d, ViewLateness %d"), ExpectedPlayers, TotalNPCs, MaxClientRoundTripSeconds, MaxClientViewLatenessSeconds);
}

void ABenchmarkGymGameModeBase::OnWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue)
{
	if (FlagName == TotalPlayerWorkerFlag)
	{
		ExpectedPlayers = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == TotalNPCsWorkerFlag)
	{
		SetTotalNPCs(FCString::Atoi(*FlagValue));
	}
	else if (FlagName == MaxRoundTripWorkerFlag)
	{
		MaxClientRoundTripSeconds = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == MaxLatenessWorkerFlag)
	{
		MaxClientViewLatenessSeconds = FCString::Atoi(*FlagValue);
	}

	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Worker flag updated - Flag %s, Value %s"), *FlagName, *FlagValue);
}

void ABenchmarkGymGameModeBase::SetTotalNPCs(int32 Value)
{
	TotalNPCs = Value;
	OnTotalNPCsUpdated(TotalNPCs);
}

void ABenchmarkGymGameModeBase::OnRepTotalNPCs()
{
	OnTotalNPCsUpdated(TotalNPCs);
}