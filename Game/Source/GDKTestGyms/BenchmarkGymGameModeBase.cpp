// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameModeBase.h"

#include "CounterComponent.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GameFramework/GameStateBase.h"
#include "GDKTestGymsGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Misc/CommandLine.h"
#include "Net/UnrealNetwork.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameModeBase);

// Metrics
namespace
{
	const FString AverageClientRTTMetricName = TEXT("UnrealAverageClientRTT");
	const FString AverageClientUpdateTimeDeltaMetricName = TEXT("UnrealAverageClientUpdateTimeDelta");
	const FString PlayersSpawnedMetricName = TEXT("UnrealActivePlayers");
	const FString AverageFPSValid = TEXT("UnrealServerFPSValid");
	const FString AverageClientFPSValid = TEXT("UnrealClientFPSValid");
	const FString ActorCountValidMetricName = TEXT("ActorCountValid");

	const FString MaxRoundTripWorkerFlag = TEXT("max_round_trip");
	const FString MaxUpdateTimeDeltaWorkerFlag = TEXT("max_update_time_delta");
	const FString MaxRoundTripCommandLineKey = TEXT("-MaxRoundTrip=");
	const FString MaxUpdateTimeDeltaCommandLineKey = TEXT("-MaxUpdateTimeDelta=");

	const FString TotalPlayerWorkerFlag = TEXT("total_players");
	const FString TotalNPCsWorkerFlag = TEXT("total_npcs");
	const FString RequiredPlayersWorkerFlag = TEXT("required_players");
	const FString TotalPlayerCommandLineKey = TEXT("-TotalPlayers=");
	const FString TotalNPCsCommandLineKey = TEXT("-TotalNPCs=");
	const FString RequiredPlayersCommandLineKey = TEXT("-RequiredPlayers=");

} // anonymous namespace

FPrintTimer::FPrintTimer(float InResetTime)
	: ResetTime(InResetTime)
	, Timer(ResetTime)
{}

void FPrintTimer::SetResetTimer(float InResetTime)
{
	ResetTime = InResetTime;
	Timer = InResetTime;
}

void FPrintTimer::Tick(float DeltaSeconds)
{
	bShouldPrint = false;
	Timer -= DeltaSeconds;
	if (Timer <= 0.0f)
	{
		Timer = ResetTime;
		bShouldPrint = true;
	}
}

FString ABenchmarkGymGameModeBase::ReadFromCommandLineKey = TEXT("ReadFromCommandLine");

ABenchmarkGymGameModeBase::ABenchmarkGymGameModeBase()
	: ExpectedPlayers(1)
	, RequiredPlayers(4096)
	, PrintMetricsTimer(10.0f)
	, AveragedClientRTTSeconds(0.0)
	, AveragedClientUpdateTimeDeltaSeconds(0.0)
	, MaxClientRoundTripSeconds(150)
	, MaxClientUpdateTimeDeltaSeconds(300)
	, bHasUxFailed(false)
	, bHasFpsFailed(false)
	, bHasDonePlayerCheck(false)
	, bHasClientFpsFailed(false)
	, bHasActorCountFailed(false)
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

	ParsePassedValues();
	BuildExpectedObjectCounts();
	TryBindWorkerFlagsDelegate();
	TryAddSpatialMetrics();
}

void ABenchmarkGymGameModeBase::TryBindWorkerFlagsDelegate()
{
	if (!GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		return;
	}

	const FString& CommandLine = FCommandLine::Get();
	if (FParse::Param(*CommandLine, *ReadFromCommandLineKey))
	{
		return;
	}

	USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
	if (ensure(SpatialDriver != nullptr))
	{
		USpatialWorkerFlags* SpatialWorkerFlags = SpatialDriver->SpatialWorkerFlags;
		if (ensure(SpatialWorkerFlags != nullptr))
		{
			FOnWorkerFlagsUpdatedBP WorkerFlagDelegate;
			WorkerFlagDelegate.BindDynamic(this, &ABenchmarkGymGameModeBase::OnWorkerFlagUpdated);
			SpatialWorkerFlags->BindToOnWorkerFlagsUpdated(WorkerFlagDelegate);
		}
	}
}

void ABenchmarkGymGameModeBase::TryAddSpatialMetrics()
{
	if (!GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		return;
	}

	USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
	if (ensure(SpatialDriver != nullptr))
	{
		USpatialMetrics* SpatialMetrics = SpatialDriver->SpatialMetrics;
		if (ensure(SpatialMetrics != nullptr))
		{
			{
				// Valid on all workers
				UserSuppliedMetric Delegate;
				Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetFPSValid);
				SpatialMetrics->SetCustomMetric(AverageFPSValid, Delegate);
			}

			if (HasAuthority())
			{
				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientRTT);
					SpatialMetrics->SetCustomMetric(AverageClientRTTMetricName, Delegate);
				}

				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientUpdateTimeDelta);
					SpatialMetrics->SetCustomMetric(AverageClientUpdateTimeDeltaMetricName, Delegate);
				}

				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetPlayersConnected);
					SpatialMetrics->SetCustomMetric(PlayersSpawnedMetricName, Delegate);
				}

				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientFPSValid);
					SpatialMetrics->SetCustomMetric(AverageClientFPSValid, Delegate);
				}

				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetActorCountValid);
					SpatialMetrics->SetCustomMetric(ActorCountValidMetricName, Delegate);
				}
			}
		}
	}
}

void ABenchmarkGymGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickServerFPSCheck(DeltaSeconds);
	TickClientFPSCheck(DeltaSeconds);
	TickPlayersConnectedCheck(DeltaSeconds);
	TickUXMetricCheck(DeltaSeconds);
	TickActorCountCheck(DeltaSeconds);

	if (HasAuthority())
	{
		PrintMetricsTimer.Tick(DeltaSeconds);
	}
}

void ABenchmarkGymGameModeBase::TickPlayersConnectedCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	// Only check players once
	if (bHasDonePlayerCheck)
	{
		return;
	}

	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);

	// This test respects the initial delay timer in both native and GDK
	if (Constants->PlayerCheckMetricDelay.IsReady())
	{
		bHasDonePlayerCheck = true;
		if (ActivePlayers < RequiredPlayers)
		{
			// This log is used by the NFR pipeline to indicate if a client failed to connect
			NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("A client connection was dropped. Required %d, got %d, num players %d"), RequiredPlayers, ActivePlayers, GetNumPlayers());
		}
		else
		{
			// Useful for NFR log inspection
			NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("All clients successfully connected. Required %d, got %d, num players %d"), RequiredPlayers, ActivePlayers, GetNumPlayers());
		}
	}
}

void ABenchmarkGymGameModeBase::TickServerFPSCheck(float DeltaSeconds)
{
	// We have already failed so no need to keep checking
	if (bHasFpsFailed)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const UGDKTestGymsGameInstance* GameInstance = GetGameInstance<UGDKTestGymsGameInstance>();
	if (GameInstance == nullptr)
	{
		return;
	}

	const UNFRConstants* Constants = UNFRConstants::Get(World);
	check(Constants);

	const float FPS = GameInstance->GetAveragedFPS();

	if (FPS < Constants->GetMinServerFPS() &&
		Constants->ServerFPSMetricDelay.IsReady())
	{
		bHasFpsFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("FPS check failed. FPS: %.8f"), FPS);
	}
}

void ABenchmarkGymGameModeBase::TickClientFPSCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	// We have already failed so no need to keep checking
	if (bHasClientFpsFailed)
	{
		return;
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

	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);

	if (!bClientFpsWasValid &&
		Constants->ClientFPSMetricDelay.IsReady())
	{
		bHasClientFpsFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("FPS check failed. A client has failed."));
	}
}

void ABenchmarkGymGameModeBase::TickUXMetricCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	int UXComponentCount = 0;
	int ValidRTTCount = 0;
	int ValidUpdateTimeDeltaCount = 0;
	float ClientRTTSeconds = 0.0f;
	float ClientUpdateTimeDeltaSeconds = 0.0f;
	for (TObjectIterator<UUserExperienceReporter> Itr; Itr; ++Itr) // These exist on player characters
	{
		UUserExperienceReporter* Component = *Itr;
		if (Component->GetOwner() != nullptr && Component->HasBegunPlay() && Component->GetWorld() == GetWorld())
		{
			if (Component->ServerRTT > 0.f)
			{
				ClientRTTSeconds += Component->ServerRTT;
				ValidRTTCount++;
			}

			if (Component->ServerUpdateTimeDelta > 0.f)
			{
				ClientUpdateTimeDeltaSeconds += Component->ServerUpdateTimeDelta;
				ValidUpdateTimeDeltaCount++;
			}

			UXComponentCount++;
		}
	}

	ActivePlayers = UXComponentCount;

	if (UXComponentCount == 0)
	{
		return; // We don't start reporting until there are some valid components in the scene.
	}

	ClientRTTSeconds /= static_cast<float>(ValidRTTCount) + 0.00001f; // Avoid div 0
	ClientUpdateTimeDeltaSeconds /= static_cast<float>(ValidUpdateTimeDeltaCount) + 0.00001f; // Avoid div 0

	AveragedClientRTTSeconds = ClientRTTSeconds;
	AveragedClientUpdateTimeDeltaSeconds = ClientUpdateTimeDeltaSeconds;

	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);

	const bool bUXMetricValid = AveragedClientRTTSeconds <= MaxClientRoundTripSeconds && AveragedClientUpdateTimeDeltaSeconds <= MaxClientUpdateTimeDeltaSeconds;

	if (!bHasUxFailed &&
		!bUXMetricValid &&
		Constants->UXMetricDelay.IsReady())
	{
		bHasUxFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("UX metric has failed. RTT: %.8f, UpdateDelta: %.8f, ActivePlayers: %d"), AveragedClientRTTSeconds, AveragedClientUpdateTimeDeltaSeconds, ActivePlayers);
	}

	if (PrintMetricsTimer.ShouldPrint())
	{
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("UX metric values. RTT: %.8f(%d), UpdateDelta: %.8f(%d), ActivePlayers: %d"), AveragedClientRTTSeconds, ValidRTTCount, AveragedClientUpdateTimeDeltaSeconds, ValidUpdateTimeDeltaCount, ActivePlayers);
	}
}

void ABenchmarkGymGameModeBase::TickActorCountCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);

	// This test respects the initial delay timer in both native and GDK
	if (!bHasActorCountFailed &&
		Constants->ActorCheckDelay.IsReady())
	{
		for (const FExpectedActorCount& ExpectedActorCount : ExpectedActorCounts)
		{
			const FString& ActorClassName = ExpectedActorCount.ActorClass->GetName();
			const int32 ExpectedCount = ExpectedActorCount.ExpectedCount;
			const int32 Variance = ExpectedActorCount.Variance;
			const int32 ActualCount = GetActorClassCount(ExpectedActorCount.ActorClass);
			bHasActorCountFailed = ActualCount < ExpectedCount - Variance || ActualCount > ExpectedCount + Variance;

			if (bHasActorCountFailed)
			{
				NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("Actor count check failed. ObjectClass %s, ExpectedCount %d, ActualCount %d"), ActorClassName, ExpectedCount, ActualCount);
				break;
			}
		}
	}
}

void ABenchmarkGymGameModeBase::ParsePassedValues()
{
	const FString& CommandLine = FCommandLine::Get();
	if (FParse::Param(*CommandLine, *ReadFromCommandLineKey))
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Found ReadFromCommandLine in command line Keys, worker flags for custom spawning will be ignored."));

		FParse::Value(*CommandLine, *TotalPlayerCommandLineKey, ExpectedPlayers);
		FParse::Value(*CommandLine, *RequiredPlayersCommandLineKey, RequiredPlayers);

		int32 NumNPCs = 0;
		FParse::Value(*CommandLine, *TotalNPCsCommandLineKey, NumNPCs);
		SetTotalNPCs(NumNPCs);

		FParse::Value(*CommandLine, *MaxRoundTripCommandLineKey, MaxClientRoundTripSeconds);
		FParse::Value(*CommandLine, *MaxUpdateTimeDeltaCommandLineKey, MaxClientUpdateTimeDeltaSeconds);
	}
	else if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString ExpectedPlayersString, RequiredPlayersString, TotalNPCsString, MaxRoundTrip, MaxUpdateTimeDelta;

		USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
		if (ensure(SpatialDriver != nullptr))
		{
			USpatialWorkerFlags* SpatialWorkerFlags = SpatialDriver->SpatialWorkerFlags;
			if (ensure(SpatialWorkerFlags != nullptr))
			{
				if (SpatialWorkerFlags->GetWorkerFlag(TotalPlayerWorkerFlag, ExpectedPlayersString))
				{
					ExpectedPlayers = FCString::Atoi(*ExpectedPlayersString);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(RequiredPlayersWorkerFlag, RequiredPlayersString))
				{
					RequiredPlayers = FCString::Atoi(*RequiredPlayersString);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(TotalNPCsWorkerFlag, TotalNPCsString))
				{
					SetTotalNPCs(FCString::Atoi(*TotalNPCsString));
				}

				if (SpatialWorkerFlags->GetWorkerFlag(MaxRoundTripWorkerFlag, MaxRoundTrip))
				{
					MaxClientRoundTripSeconds = FCString::Atoi(*MaxRoundTrip);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(MaxUpdateTimeDeltaWorkerFlag, MaxUpdateTimeDelta))
				{
					MaxClientUpdateTimeDeltaSeconds = FCString::Atoi(*MaxUpdateTimeDelta);
				}
			}
		}
	}

	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Players %d, NPCs %d, RoundTrip %d, UpdateTimeDelta %d"), ExpectedPlayers, TotalNPCs, MaxClientRoundTripSeconds, MaxClientUpdateTimeDeltaSeconds);
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
	else if (FlagName == MaxUpdateTimeDeltaWorkerFlag)
	{
		MaxClientUpdateTimeDeltaSeconds = FCString::Atoi(*FlagValue);
	}

	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Worker flag updated - Flag %s, Value %s"), *FlagName, *FlagValue);
}

void ABenchmarkGymGameModeBase::SetTotalNPCs(int32 Value)
{
	if (Value != TotalNPCs)
	{
		TotalNPCs = Value;
		OnTotalNPCsUpdated(TotalNPCs);
	}
}

void ABenchmarkGymGameModeBase::OnRepTotalNPCs()
{
	OnTotalNPCsUpdated(TotalNPCs);
}

int32 ABenchmarkGymGameModeBase::GetActorClassCount(TSubclassOf<AActor> ActorClass) const
{
	const UCounterComponent* CounterComponent = Cast<UCounterComponent>(GameState->GetComponentByClass(UCounterComponent::StaticClass()));
	check(CounterComponent != nullptr)

	return CounterComponent->GetActorClassCount(ActorClass);
}