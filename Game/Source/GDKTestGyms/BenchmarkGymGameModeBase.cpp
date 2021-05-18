// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameModeBase.h"

#include "CounterComponent.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/MovementComponent.h"
#include "GDKTestGymsGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
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

	const FString MemReportFlag = TEXT("mem_report");
	const FString MemRemportIntervalKey = TEXT("-MemReportInterval=");
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
	, TickActorCountTimer(60) // 1-minutes to allow workers to get setup and the deployment to get into a stable state
	, bHasRequiredPlayersCheckFailed(false)
	, RequiredPlayerCheckTimer(11*60) // 1-minute later then RequiredPlayerReportTimer to make sure all the workers had reported their migration
	, DeploymentValidTimer(16*60) // 16-minute window to check between
	, NumWorkers(1)
	, NumSpawnZones(1)
#if	STATS
	, StatStartFileTimer(60 * 60 * 24)
	, StatStopFileTimer(60)
	, MemReportIntervalTimer(60 * 60 * 24)
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
	AddExpectedActorCount(SimulatedPawnClass, RequiredPlayers, 1);
}

void ABenchmarkGymGameModeBase::AddExpectedActorCount(const TSubclassOf<AActor>& ActorClass, int32 ExpectedCount, int32 Variance)
{
	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Adding NFR actor count expectation - ActorClass: %s, ExpectedCount: %d, Variance: %d"), *ActorClass->GetName(), ExpectedCount, Variance);
	ExpectedActorCounts.Add(ActorClass, FExpectedActorCountConfig(ExpectedCount, Variance));
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
#if	STATS
	if (CPUProfileInterval > 0)
	{
		if (StatStartFileTimer.HasTimerGoneOff())
		{
			FString Cmd(TEXT("stat startfile"));
			if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
			{
				USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
				if (ensure(SpatialDriver != nullptr))
				{
					FString InFileName = FString::Printf(TEXT("%s-%s"), *SpatialDriver->Connection->GetWorkerId(), *FDateTime::Now().ToString(TEXT("%m.%d-%H.%M.%S")));
					const FString Filename = CreateProfileFilename(InFileName, TEXT(".ue4stats"), true);
					Cmd.Append(FString::Printf(TEXT(" %s"), *Filename));
				}
			}
			GEngine->Exec(GetWorld(), *Cmd);
			StatStartFileTimer.SetTimer(CPUProfileInterval);
		}
		if (StatStopFileTimer.HasTimerGoneOff())
		{
			GEngine->Exec(GetWorld(), TEXT("stat stopfile"));
			StatStopFileTimer.SetTimer(CPUProfileInterval);
		}
	}

	if (MemReportInterval > 0 && MemReportIntervalTimer.HasTimerGoneOff())
	{
		FString Cmd = TEXT("memreport -full");
		if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
		{
			USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(GetNetDriver());
			if (ensure(SpatialDriver != nullptr))
			{
				Cmd.Append(FString::Printf(TEXT(" NAME=%s-%s"), *SpatialDriver->Connection->GetWorkerId(), *FDateTime::Now().ToString(TEXT("%m.%d-%H.%M.%S"))));
			}
		}
		GEngine->Exec(nullptr, *Cmd);
		MemReportIntervalTimer.SetTimer(MemReportInterval);
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
		const FActorCountInfo* ActorCountInfo = TotalActorCounts.Find(SimulatedPawnClass);
		if (ActorCountInfo != nullptr)
		{
			int SmoothedTotalAuthPlayers = ActorCountInfo->GetSmoothedTotal();
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
	// This test respects the initial delay timer in both native and GDK
	if (TickActorCountTimer.HasTimerGoneOff())
	{
		TryInitialiseExpectedActorCounts();

		const UWorld* World = GetWorld();
		const FString WorkerID = GetGameInstance()->GetSpatialWorkerId();
		if (WorkerID.IsEmpty())
		{
			return;
		}

		ActorCountMap& ThisWorkerActorCounts = WorkerActorCounts.FindOrAdd(WorkerID);

		for (auto const& Pair : ExpectedActorCounts)
		{
			const TSubclassOf<AActor>& ActorClass = Pair.Key;
			if (!USpatialStatics::IsActorGroupOwnerForClass(World, ActorClass))
			{
				continue;
			}

			const FExpectedActorCountConfig& Config = Pair.Value;
			if (Config.ExpectedCount > 0)
			{
				const int32 ActorCount = GetActorAuthCount(ActorClass);

				FActorCountInfo* ActorCountInfo = ThisWorkerActorCounts.Find(ActorClass);
				bool bActorCountInfoMissing = ActorCountInfo == nullptr;
				if (bActorCountInfoMissing)
				{
					ActorCountInfo = &ThisWorkerActorCounts.Add(ActorClass);
				}

				if (bActorCountInfoMissing || ActorCountInfo->GetTotal() != ActorCount)
				{
					ReportAuthoritativeActorCount(WorkerID, ActorClass, ActorCount);
					ActorCountInfo->SetTotal(ActorCount);
				}
			}
		}

		const int32 TickActorCountPeriod = 1; /*seconds*/
		TickActorCountTimer.SetTimer(TickActorCountPeriod);
	}

	if (HasAuthority())
	{
		CheckTotalCountsForActors();
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
				FString CPUProfileString;
				if (SpatialWorkerFlags->GetWorkerFlag(StatProfileWorkerFlag, CPUProfileString))
				{
					InitStatTimer(CPUProfileString);
				}

				FString MemReportIntervalString;
				if (SpatialWorkerFlags->GetWorkerFlag(MemReportFlag,MemReportIntervalString))
				{
					InitMemReportTimer(MemReportIntervalString);
				}
#endif
			}
		}
	}

	//Move profile configuration outside to avoid conflict with worker flag
#if	STATS
	FString CPUProfileString;
	FParse::Value(*CommandLine, *StatProfileCommandLineKey, CPUProfileString);
	InitStatTimer(CPUProfileString);

	FString MemReportIntervalString;
	FParse::Value(*CommandLine, *MemRemportIntervalKey, MemReportIntervalString);
	InitMemReportTimer(MemReportIntervalString);
#endif
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
		InitStatTimer(FlagValue);
	}
	else if (FlagName == MemReportFlag)
	{
		InitMemReportTimer(FlagValue);
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

int32 ABenchmarkGymGameModeBase::GetActorAuthCount(const TSubclassOf<AActor>& ActorClass) const
{
	const UCounterComponent* CounterComponent = Cast<UCounterComponent>(GameState->GetComponentByClass(UCounterComponent::StaticClass()));
	check(CounterComponent != nullptr)
	return CounterComponent->GetActorAuthCount(ActorClass);
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
				ReportMigration(GetGameInstance()->GetSpatialWorkerId(), AverageMigrationOfCurrentWorkerPerSecond);
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
void ABenchmarkGymGameModeBase::InitStatTimer(const FString& CPUProfileString)
{
	FString CPUProfileIntervalString, CPUProfileDurationString;
	if (CPUProfileString.Split(TEXT("&"), &CPUProfileIntervalString, &CPUProfileDurationString))
	{
		int32 FirstStartCPUProfile = FCString::Atoi(*CPUProfileIntervalString);
		int32 CPUProfileDuration = FCString::Atoi(*CPUProfileDurationString);
		StatStartFileTimer.SetTimer(FirstStartCPUProfile);
		StatStopFileTimer.SetTimer(FirstStartCPUProfile + CPUProfileDuration);
		CPUProfileInterval = FirstStartCPUProfile + CPUProfileDuration;
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("CPU profile interval is set to %ds, duration is set to %ds"), FirstStartCPUProfile, CPUProfileDuration);
	}
	else
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Please ensure both CPU profile interval and duration are set properly"));
	}
}

void ABenchmarkGymGameModeBase::InitMemReportTimer(const FString& MemReportIntervalString)
{
	MemReportInterval = FCString::Atoi(*MemReportIntervalString);
	MemReportIntervalTimer.SetTimer(MemReportInterval);
	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("MemReport Interval is set to %d seconds"), MemReportInterval);
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

void ABenchmarkGymGameModeBase::ReportAuthoritativeActorCount_Implementation(const FString& WorkerID, TSubclassOf<AActor> ActorClass, int32 ActorCount)
{
	ActorCountMap& SpecificWorkerActorCounts = WorkerActorCounts.FindOrAdd(WorkerID);
	FActorCountInfo& ActorCountInfo = SpecificWorkerActorCounts.FindOrAdd(ActorClass);
	ActorCountInfo.SetTotal(ActorCount);
}

void ABenchmarkGymGameModeBase::CheckTotalCountsForActors()
{
	const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
	check(Constants);

	const bool bLogActorCountDetails = PrintMetricsTimer.HasTimerGoneOff();
	const bool bIsReadyToConsiderActorCount = WorkerActorCounts.Num() == NumWorkers &&
		Constants->ActorCheckDelay.HasTimerGoneOff() &&
		!TestLifetimeTimer.HasTimerGoneOff();

	if (!bIsReadyToConsiderActorCount && bLogActorCountDetails)
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Not ready to consider actor count metric"));
	}

	TMap<TSubclassOf<AActor>, int32> TempTotalActorCounts;
	for (const auto& WorkerPair : WorkerActorCounts)
	{
		const FString WorkerId = WorkerPair.Key;
		const ActorCountMap& SpecificWorkerActorCounts = WorkerPair.Value;

		if (bLogActorCountDetails)
		{
			UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("--- Actor Count for Worker: %s ---"), *WorkerId);
		}

		for (const auto& ActorCountPair : SpecificWorkerActorCounts)
		{
			const TSubclassOf<AActor>& ActorClass = ActorCountPair.Key;
			const FActorCountInfo& ActorCountInfo = ActorCountPair.Value;

			int32 WorkerActorCount = ActorCountInfo.GetTotal();
			int32& TotalActorCount = TempTotalActorCounts.FindOrAdd(ActorClass);
			TotalActorCount += WorkerActorCount;

			if (bLogActorCountDetails)
			{
				UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Class: %s, Total: %d, Smoothed: %d"), *ActorClass->GetName(), WorkerActorCount, ActorCountInfo.GetSmoothedTotal());
			}
		}
	}

	if (bLogActorCountDetails && TempTotalActorCounts.Num() > 0)
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("--- Actor Count Totals ---"));
	}

	for (const auto& ActorCountPair : TempTotalActorCounts)
	{
		const TSubclassOf<AActor>& ActorClass = ActorCountPair.Key;
		const int32 TotalActorCount = ActorCountPair.Value;

		FActorCountInfo& TotalActorCountInfo = TotalActorCounts.FindOrAdd(ActorClass);
		TotalActorCountInfo.SetTotal(TotalActorCount);

		if (bLogActorCountDetails)
		{
			UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Class: %s, Total: %d, Smoothed: %d"), *ActorClass->GetName(), TotalActorCount, TotalActorCountInfo.GetSmoothedTotal());
		}

		if (bIsReadyToConsiderActorCount)
		{
			// Check for test failure
			const FExpectedActorCountConfig& ExpectedActorCount = ExpectedActorCounts[ActorClass];
			bActorCountFailureState = abs(TotalActorCount - ExpectedActorCount.ExpectedCount) > ExpectedActorCount.Variance;
			if (bActorCountFailureState && !bHasActorCountFailed)
			{
				bHasActorCountFailed = true;
				NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: Unreal actor count check. ObjectClass %s, ExpectedCount %d, ActualCount %d"),
					*NFRFailureString,
					*ActorClass->GetName(),
					ExpectedActorCount.ExpectedCount,
					TotalActorCount);
			}
		}
	}
}