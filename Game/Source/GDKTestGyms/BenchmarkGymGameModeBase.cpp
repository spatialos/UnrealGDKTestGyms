// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameModeBase.h"

#include "CounterComponent.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/MovementComponent.h"
#include "GDKTestGymsGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Misc/CommandLine.h"
#include "Net/UnrealNetwork.h"
#include "UserExperienceComponent.h"
#include "Utils/SpatialMetrics.h"
#include "Utils/SpatialStatics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameModeBase);

// Metrics
namespace
{
	const FString AverageClientRTTMetricName = TEXT("UnrealAverageClientRTT");
	const FString AverageClientUpdateTimeDeltaMetricName = TEXT("UnrealAverageClientUpdateTimeDelta");
	const FString ExpectedPlayersValidMetricName = TEXT("ExpectedPlayersValid");
	const FString ActorMigrationValidMetricName = TEXT("UnrealActorMigration");
	const FString AverageFPSValid = TEXT("UnrealServerFPSValid");
	const FString AverageClientFPSValid = TEXT("UnrealClientFPSValid");
	const FString ActorCountValidMetricName = TEXT("UnrealActorCountValid");

	const FString MaxRoundTripWorkerFlag = TEXT("max_round_trip");
	const FString MaxUpdateTimeDeltaWorkerFlag = TEXT("max_update_time_delta");
	const FString MaxRoundTripCommandLineKey = TEXT("-MaxRoundTrip=");
	const FString MaxUpdateTimeDeltaCommandLineKey = TEXT("-MaxUpdateTimeDelta=");

	const FString TestLiftimeWorkerFlag = TEXT("test_lifetime");
	const FString TestLiftimeCommandLineKey = TEXT("-TestLifetime=");

	const FString TotalPlayerWorkerFlag = TEXT("total_players");
	const FString TotalNPCsWorkerFlag = TEXT("total_npcs");
	const FString RequiredPlayersWorkerFlag = TEXT("required_players");
	const FString TotalPlayerCommandLineKey = TEXT("-TotalPlayers=");
	const FString TotalNPCsCommandLineKey = TEXT("-TotalNPCs=");
	const FString RequiredPlayersCommandLineKey = TEXT("-RequiredPlayers=");

	const FString MinActorMigrationWorkerFlag = TEXT("min_actor_migration");
	const FString MinActorMigrationCommandLineKey = TEXT("-MinActorMigration=");
	
	const FString NumWorkersWorkerFlag = TEXT("num_workers");
	const FString NumWorkersCommandLineKey = TEXT("-NumWorkers=");

	const FString NumSpawnZonesWorkerFlag = TEXT("num_spawn_zones");
	const FString NumSpawnZonesCommandLineKey = TEXT("-NumSpawnZones=");
#if	STATS
	const FString StatProfileWorkerFlag = TEXT("stat_profile");
	const FString StatProfileCommandLineKey = TEXT("-StatProfile=");
#endif
	const FString NFRFailureString = TEXT("NFR scenario failed");

} // anonymous namespace

FString ABenchmarkGymGameModeBase::ReadFromCommandLineKey = TEXT("ReadFromCommandLine");

ABenchmarkGymGameModeBase::ABenchmarkGymGameModeBase()
	: ExpectedPlayers(1)
	, RequiredPlayers(4096)
	, AveragedClientRTTMS(0.0)
	, AveragedClientUpdateTimeDeltaMS(0.0)
	, MaxClientRoundTripMS(150)
	, MaxClientUpdateTimeDeltaMS(300)
	, bHasUxFailed(false)
	, bHasFpsFailed(false)
	, bHasClientFpsFailed(false)
	, bHasActorCountFailed(false)
	, bActorCountFailureState(false)
	, bExpectedActorCountsInitialised(false)
	, bHasActorMigrationCheckFailed(false)
	, PreviousTickMigration(0)
	, UXAuthActorCount(0)
	, MigrationOfCurrentWorker(0)
	, MigrationCountSeconds(0.0)
	, MigrationWindowSeconds(5*60.0f)
	, MinActorMigrationPerSecond(0.0)
	, ActorMigrationReportTimer(1)
	, ActorMigrationCheckTimer(11*60) // 1-minute later then ActorMigrationCheckDelay + MigrationWindowSeconds to make sure all the workers had reported their migration	
	, ActorMigrationCheckDelay(5*60)
	, PrintMetricsTimer(10)
	, TestLifetimeTimer(0)
	, bHasRequiredPlayersCheckFailed(false)
	, SmoothedTotalAuthPlayers(-1.0f)
	, RequiredPlayerReportTimer(10 * 60)
	, RequiredPlayerCheckTimer(11*60) // 1-minute later then RequiredPlayerReportTimer to make sure all the workers had reported their migration
	, DeploymentValidTimer(16*60) // 16-minute window to check between
	, NoneAuthPlayerControllerTimer(60)
	, NumWorkers(1)
	, NumSpawnZones(1)
#if	STATS
	, StatStartFileTimer(60 * 60 * 24)
	, StatStopFileTimer(60)
#endif
{
	PrimaryActorTick.bCanEverTick = true;

	if (USpatialStatics::IsSpatialNetworkingEnabled())
	{
		bAlwaysRelevant = true;
	}
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
	TryBindWorkerFlagsDelegate();
	TryAddSpatialMetrics();
}

void ABenchmarkGymGameModeBase::TryInitialiseExpectedActorCounts()
{
	if (!bExpectedActorCountsInitialised)
	{
		BuildExpectedActorCounts();
		bExpectedActorCountsInitialised = true;
	}
}

void ABenchmarkGymGameModeBase::BuildExpectedActorCounts()
{
	AddExpectedActorCount(NPCClass, TotalNPCs, 1);
	AddExpectedActorCount(SimulatedPawnClass, ExpectedPlayers, 1);
}

void ABenchmarkGymGameModeBase::AddExpectedActorCount(TSubclassOf<AActor> ActorClass, int32 ExpectedCount, int32 Variance)
{
	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Adding NFR actor count expectation - ActorClass: %s, ExpectedCount: %d, Variance: %d"), *ActorClass->GetName(), ExpectedCount, Variance);
	ExpectedActorCounts.Add(FExpectedActorCount(ActorClass, ExpectedCount, Variance));
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
			FOnAnyWorkerFlagUpdatedBP WorkerFlagDelegate;
			WorkerFlagDelegate.BindDynamic(this, &ABenchmarkGymGameModeBase::OnAnyWorkerFlagUpdated);
			SpatialWorkerFlags->RegisterAnyFlagUpdatedCallback(WorkerFlagDelegate);
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

			{
				UserSuppliedMetric Delegate;
				Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetActorCountValid);
				SpatialMetrics->SetCustomMetric(ActorCountValidMetricName, Delegate);
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
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetRequiredPlayersValid);
					SpatialMetrics->SetCustomMetric(ExpectedPlayersValidMetricName, Delegate);
				}

				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetClientFPSValid);
					SpatialMetrics->SetCustomMetric(AverageClientFPSValid, Delegate);
				}

				{
					UserSuppliedMetric Delegate;
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetTotalMigrationValid);
					SpatialMetrics->SetCustomMetric(ActorMigrationValidMetricName, Delegate);
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
	TickActorMigration(DeltaSeconds);
	
	// PrintMetricsTimer needs to be reset at the the end of ABenchmarkGymGameModeBase::Tick.
	// This is so that the above function have a chance to run logic dependant on PrintMetricsTimer.HasTimerGoneOff().
	if (HasAuthority() &&
		PrintMetricsTimer.HasTimerGoneOff())
	{
		PrintMetricsTimer.SetTimer(10);
	}
	if (NoneAuthPlayerControllerTimer.HasTimerGoneOff())
	{
		int32 Count = 0;
		for (FConstPlayerControllerIterator PCIt = GetWorld()->GetPlayerControllerIterator(); PCIt; ++PCIt)
		{
			if (APlayerController* PC = PCIt->Get())
			{
				if (false == PC->HasAuthority())
				{
					++Count;
				}
			}
		}
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("None authority PlayerController:%d"), Count);
		NoneAuthPlayerControllerTimer.SetTimer(60);
	}
#if	STATS
	if (StatStartFileTimer.HasTimerGoneOff())
	{
		FString Cmd(TEXT("stat startfile"));
		if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
		{
			USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
			if (ensure(SpatialDriver != nullptr))
			{
				Cmd = FString::Printf(TEXT("stat startfile %s.ue4stats"), *SpatialDriver->Connection->GetWorkerId());
			}
		}
		GEngine->Exec(GetWorld(), *Cmd);
		StatStartFileTimer.SetTimer(999999);
	}
	if (StatStopFileTimer.HasTimerGoneOff())
	{
		GEngine->Exec(GetWorld(), TEXT("stat stopfile"));
		StatStopFileTimer.SetTimer(999999);
	}
#endif
}

void ABenchmarkGymGameModeBase::TickPlayersConnectedCheck(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	// Only check players once
	if (bHasRequiredPlayersCheckFailed)
	{
		return;
	}

	if (RequiredPlayerCheckTimer.HasTimerGoneOff() && !DeploymentValidTimer.HasTimerGoneOff())
	{
		if (SmoothedTotalAuthPlayers < RequiredPlayers)
		{
			bHasRequiredPlayersCheckFailed = true;
			// This log is used by the NFR pipeline to indicate if a client failed to connect
			NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: Client connection dropped. Required %d, got %.1f"), *NFRFailureString, RequiredPlayers, SmoothedTotalAuthPlayers);
		}
		else
		{
			RequiredPlayerCheckTimer.SetTimer(10);
			// Useful for NFR log inspection
			NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("All clients successfully connected. Required %d, got %.1f"), RequiredPlayers, SmoothedTotalAuthPlayers);
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
		Constants->ServerFPSMetricDelay.HasTimerGoneOff())
	{
		bHasFpsFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("%s: Server FPS check. FPS: %.8f"), *NFRFailureString, FPS);
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
		Constants->ClientFPSMetricDelay.HasTimerGoneOff())
	{
		bHasClientFpsFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("%s: Client FPS check."), *NFRFailureString);
	}
}

void ABenchmarkGymGameModeBase::TickUXMetricCheck(float DeltaSeconds)
{
	UXAuthActorCount = 0;
	int ValidRTTCount = 0;
	int ValidUpdateTimeDeltaCount = 0;
	float ClientRTTMS = 0.0f;
	float ClientUpdateTimeDeltaMS = 0.0f;
	for (TObjectIterator<UUserExperienceReporter> Itr; Itr; ++Itr) // These exist on player characters
	{
		UUserExperienceReporter* Component = *Itr;
		if (Component->GetOwner() != nullptr && Component->HasBegunPlay() && Component->GetWorld() == GetWorld())
		{
			if (Component->ServerRTTMS > 0.f)
			{
				ClientRTTMS += Component->ServerRTTMS;
				ValidRTTCount++;
			}

			if (Component->ServerUpdateTimeDeltaMS > 0.f)
			{
				ClientUpdateTimeDeltaMS += Component->ServerUpdateTimeDeltaMS;
				ValidUpdateTimeDeltaCount++;
			}

			if (Component->GetOwner()->HasAuthority())
			{
				UXAuthActorCount++;
			}
		}
	}	

	if (RequiredPlayerReportTimer.HasTimerGoneOff())
	{
		// We don't start reporting until RequiredPlayerReportTimer has gone off
		ReportAuthoritativePlayers(FPlatformProcess::ComputerName(), GetPlayerControllerCount());
		RequiredPlayerReportTimer.SetTimer(1);
	}

	if (!HasAuthority())
	{
		return;
	}

	ClientRTTMS /= static_cast<float>(ValidRTTCount) + 0.00001f; // Avoid div 0
	ClientUpdateTimeDeltaMS /= static_cast<float>(ValidUpdateTimeDeltaCount) + 0.00001f; // Avoid div 0

	AveragedClientRTTMS = ClientRTTMS;
	AveragedClientUpdateTimeDeltaMS = ClientUpdateTimeDeltaMS;

	const bool bUXMetricValid = AveragedClientRTTMS <= MaxClientRoundTripMS && AveragedClientUpdateTimeDeltaMS <= MaxClientUpdateTimeDeltaMS;
	
	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);
	if (!bHasUxFailed &&
		!bUXMetricValid &&
		Constants->UXMetricDelay.HasTimerGoneOff())
	{
		bHasUxFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: UX metric check. RTT: %.8f, UpdateDelta: %.8f"), *NFRFailureString, AveragedClientRTTMS, AveragedClientUpdateTimeDeltaMS);
	}

	if (PrintMetricsTimer.HasTimerGoneOff())
	{
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("UX metric values. RTT: %.8f(%d), UpdateDelta: %.8f(%d)"), AveragedClientRTTMS, ValidRTTCount, AveragedClientUpdateTimeDeltaMS, ValidUpdateTimeDeltaCount);
	}
}

void ABenchmarkGymGameModeBase::TickActorCountCheck(float DeltaSeconds)
{
	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);

	// This test respects the initial delay timer in both native and GDK
	if (Constants->ActorCheckDelay.HasTimerGoneOff() &&
		!TestLifetimeTimer.HasTimerGoneOff())
	{
		TryInitialiseExpectedActorCounts();

		const UWorld* World = GetWorld();
		for (const FExpectedActorCount& ExpectedActorCount : ExpectedActorCounts)
		{
			if (!USpatialStatics::IsActorGroupOwnerForClass(World, ExpectedActorCount.ActorClass))
			{
				continue;
			}

			const int32 ExpectedCount = ExpectedActorCount.ExpectedCount;
			const int32 Variance = ExpectedActorCount.Variance;
			const int32 ActualCount = GetActorClassCount(ExpectedActorCount.ActorClass);
			bActorCountFailureState = abs(ActualCount - ExpectedCount) > Variance;

			if (bActorCountFailureState)
			{
				if (!bHasActorCountFailed)
				{
					bHasActorCountFailed = true;
					NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: Unreal actor count check. ObjectClass %s, ExpectedCount %d, ActualCount %d"),
						*NFRFailureString,
						*ExpectedActorCount.ActorClass->GetName(),
						ExpectedCount,
						ActualCount);
				}
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

		int32 Lifetime = 0;
		FParse::Value(*CommandLine, *TestLiftimeCommandLineKey, Lifetime);
		SetLifetime(Lifetime);

		FParse::Value(*CommandLine, *MaxRoundTripCommandLineKey, MaxClientRoundTripMS);
		FParse::Value(*CommandLine, *MaxUpdateTimeDeltaCommandLineKey, MaxClientUpdateTimeDeltaMS);
		FParse::Value(*CommandLine, *MinActorMigrationCommandLineKey, MinActorMigrationPerSecond);
		FParse::Value(*CommandLine, *NumWorkersCommandLineKey, NumWorkers);
		FParse::Value(*CommandLine, *NumSpawnZonesCommandLineKey, NumSpawnZones);
		
#if	STATS
		FString StatProfileString;
		FParse::Value(*CommandLine, *StatProfileCommandLineKey, StatProfileString);
		SetStatTimer(StatProfileString);
#endif
	}
	else if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString ExpectedPlayersString, RequiredPlayersString, TotalNPCsString, MaxRoundTrip, MaxUpdateTimeDelta, LifetimeString, MinActorMigrationString, NumWorkersString, NumSpawnZonesString;

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
					MaxClientRoundTripMS = FCString::Atoi(*MaxRoundTrip);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(MaxUpdateTimeDeltaWorkerFlag, MaxUpdateTimeDelta))
				{
					MaxClientUpdateTimeDeltaMS = FCString::Atoi(*MaxUpdateTimeDelta);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(TestLiftimeWorkerFlag, LifetimeString))
				{
					SetLifetime(FCString::Atoi(*LifetimeString));
				}
				
				if (SpatialWorkerFlags->GetWorkerFlag(MinActorMigrationWorkerFlag, MinActorMigrationString))
				{
					MinActorMigrationPerSecond = FCString::Atof(*MinActorMigrationString);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(NumWorkersWorkerFlag, NumWorkersString))
				{
					NumWorkers = FCString::Atoi(*NumWorkersString);
				}

				if (SpatialWorkerFlags->GetWorkerFlag(NumSpawnZonesWorkerFlag, NumSpawnZonesString))
				{
					NumSpawnZones = FCString::Atoi(*NumSpawnZonesString);
				}
#if	STATS
				FString StatProfileString;
				if (SpatialWorkerFlags->GetWorkerFlag(StatProfileWorkerFlag, StatProfileString))
				{
					SetStatTimer(StatProfileString);
				}
#endif
			}
		}
	}

	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Players %d, NPCs %d, RoundTrip %d, UpdateTimeDelta %d, MinActorMigrationPerSecond %.8f, NumWorkers %d, NumSpawnZones %d"), 
		ExpectedPlayers, TotalNPCs, MaxClientRoundTripMS, MaxClientUpdateTimeDeltaMS, MinActorMigrationPerSecond, NumWorkers, NumSpawnZones);
}

void ABenchmarkGymGameModeBase::OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue)
{
	if (FlagName == TotalPlayerWorkerFlag)
	{
		ExpectedPlayers = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == RequiredPlayersWorkerFlag)
	{
		RequiredPlayers = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == TotalNPCsWorkerFlag)
	{
		SetTotalNPCs(FCString::Atoi(*FlagValue));
	}
	else if (FlagName == MaxRoundTripWorkerFlag)
	{
		MaxClientRoundTripMS = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == MaxUpdateTimeDeltaWorkerFlag)
	{
		MaxClientUpdateTimeDeltaMS = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == TestLiftimeWorkerFlag)
	{
		SetLifetime(FCString::Atoi(*FlagValue));
	}
	else if (FlagName == MinActorMigrationWorkerFlag)
	{
		MinActorMigrationPerSecond = FCString::Atof(*FlagValue);
	}
	else if (FlagName == NumWorkersWorkerFlag)
	{
		NumWorkers = FCString::Atof(*FlagValue);
	}
	else if (FlagName == NumSpawnZonesWorkerFlag)
	{
		NumSpawnZones = FCString::Atof(*FlagValue);
	}
#if	STATS
	else if (FlagName == StatProfileWorkerFlag)
	{
		SetStatTimer(FlagValue);
	}
#endif

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

void ABenchmarkGymGameModeBase::SetLifetime(int32 Lifetime)
{
	if (TestLifetimeTimer.SetTimer(Lifetime))
	{
		TestLifetimeTimer.SetLock(true);
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Setting NFR test lifetime %d"), Lifetime);
	}
	else
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Warning, TEXT("Could not set NFR test liftime to %d. Timer was locked."), Lifetime);
	}
}

void ABenchmarkGymGameModeBase::ReportAuthoritativePlayers_Implementation(const FString& WorkerID, const int AuthoritativePlayers)
{
	if (HasAuthority())
	{
		MapAuthoritativePlayers.Emplace(WorkerID, AuthoritativePlayers);
		int32 TotalPlayers = 0;
		for (const auto& KeyValue : MapAuthoritativePlayers)
		{
			TotalPlayers += KeyValue.Value;
		}
		if (MapAuthoritativePlayers.Num() == NumWorkers)
		{
			if (SmoothedTotalAuthPlayers < 0.0f)
			{
				SmoothedTotalAuthPlayers = TotalPlayers;
			}
			else
			{
				SmoothedTotalAuthPlayers = SmoothedTotalAuthPlayers * 0.9f + TotalPlayers * 0.1;
			}
			UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("ReportAuthoritativePlayers(%s, %d) Total:%.1f"),
				*WorkerID, AuthoritativePlayers, SmoothedTotalAuthPlayers);
		}
	}
}

void ABenchmarkGymGameModeBase::TickActorMigration(float DeltaSeconds)
{
	if (bHasActorMigrationCheckFailed)
	{
		return;
	}

	// This test respects the initial delay timer only for multiworker
	if (ActorMigrationCheckDelay.HasTimerGoneOff())
	{
		// Count how many actors hand over authority in 1 tick
		int Delta = FMath::Abs(UXAuthActorCount - PreviousTickMigration);
		MigrationDeltaHistory.Enqueue(MigrationDeltaPair(Delta, DeltaSeconds));
		bool bChanged = false;
		if (MigrationCountSeconds > MigrationWindowSeconds)
		{
			MigrationDeltaPair OldestValue;
			if (MigrationDeltaHistory.Dequeue(OldestValue))
			{
				MigrationOfCurrentWorker -= OldestValue.Key;
				MigrationSeconds -= OldestValue.Value;
			}
		}
		MigrationOfCurrentWorker += Delta;
		MigrationSeconds += DeltaSeconds;
		PreviousTickMigration = UXAuthActorCount;

		if (MigrationCountSeconds > MigrationWindowSeconds)
		{
			if (ActorMigrationReportTimer.HasTimerGoneOff())
			{
				// Only report AverageMigrationOfCurrentWorkerPerSecond to the worker which has authority
				float AverageMigrationOfCurrentWorkerPerSecond = MigrationOfCurrentWorker / MigrationSeconds;
				ReportMigration(FPlatformProcess::ComputerName(), AverageMigrationOfCurrentWorkerPerSecond);
				ActorMigrationReportTimer.SetTimer(1);
			}

			if (HasAuthority() && ActorMigrationCheckTimer.HasTimerGoneOff())
			{
				float Migration = 0.0f;
				for (const auto& KeyValue : MapWorkerActorMigration)
				{
					Migration += KeyValue.Value;
				}
				if (Migration < MinActorMigrationPerSecond)
				{
					bHasActorMigrationCheckFailed = true;
					NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: Actor migration check failed. Migration=%.8f MinActorMigrationPerSecond=%.8f MigrationExactlyWindowSeconds=%.8f"),
						*NFRFailureString, Migration, MinActorMigrationPerSecond, MigrationSeconds);
				}
				else
				{
					// Reset timer for next check after 10s
					ActorMigrationCheckTimer.SetTimer(10);
					UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Actor migration check TotalMigrations=%.8f MinActorMigrationPerSecond=%.8f MigrationExactlyWindowSeconds=%.8f"),
						Migration, MinActorMigrationPerSecond, MigrationSeconds);
				}
			}
		}
		MigrationCountSeconds += DeltaSeconds;
	}
}

void ABenchmarkGymGameModeBase::ReportMigration_Implementation(const FString& WorkerID, const float Migration)
{
	if (HasAuthority())
	{
		MapWorkerActorMigration.Emplace(WorkerID, Migration);
	}
}
#if	STATS
void ABenchmarkGymGameModeBase::SetStatTimer(const FString& TimeString)
{
	FString StartDelayString, DurationString;
	if (TimeString.Split(TEXT(","), &StartDelayString, &DurationString))
	{
		int32 StartDelay = FCString::Atoi(*StartDelayString);
		int32 Duration = FCString::Atoi(*DurationString);
		StatStartFileTimer.SetTimer(StartDelay);
		StatStopFileTimer.SetTimer(StartDelay + Duration);
	}
}
#endif

int32 ABenchmarkGymGameModeBase::GetPlayerControllerCount() const
{
	int32 Count = 0;
	for (FConstPlayerControllerIterator PCIt = GetWorld()->GetPlayerControllerIterator(); PCIt; ++PCIt)
	{
		if (APlayerController* PC = PCIt->Get())
		{
			if (PC->HasAuthority())
			{
				++Count;
			}
		}
	}
	return Count;
}
