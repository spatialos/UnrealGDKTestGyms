// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameModeBase.h"

#include "CounterComponent.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/MovementComponent.h"
#include "GDKTestGymsGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Net/UnrealNetwork.h"
#include "Utils/SpatialMetrics.h"
#include "Utils/SpatialStatics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameModeBase);

// Metrics
namespace
{
	const FString AverageClientRTTMetricName = TEXT("UnrealAverageClientRTT");
	const FString AverageClientUpdateTimeDeltaMetricName = TEXT("UnrealAverageClientUpdateTimeDelta");
	const FString PlayersSpawnedMetricName = TEXT("UnrealActivePlayers");
	const FString HandOverActorsMetricName = TEXT("UnrealHandOverActors");
	const FString AverageFPSValid = TEXT("UnrealServerFPSValid");
	const FString AverageClientFPSValid = TEXT("UnrealClientFPSValid");
	const FString ActorCountValidMetricName = TEXT("UnrealActorCountValid");

	const FString MaxRoundTripWorkerFlag = TEXT("max_round_trip");
	const FString MaxUpdateTimeDeltaWorkerFlag = TEXT("max_update_time_delta");
	const FString MaxRoundTripCommandLineKey = TEXT("-MaxRoundTrip=");
	const FString MaxUpdateTimeDeltaCommandLineKey = TEXT("-MaxUpdateTimeDelta=");

	const FString TestLiftimeWorkerFlag = TEXT("test_lifetime");
	const FString TestLiftimeCommandLineKey = TEXT("-TestLifetime=");

	const FString HandOverTotalFrameCountWorkerFlag = TEXT("HandOver_total_frame_count");
	const FString TotalPlayerWorkerFlag = TEXT("total_players");
	const FString TotalNPCsWorkerFlag = TEXT("total_npcs");
	const FString RequiredPlayersWorkerFlag = TEXT("required_players");
	const FString TotalPlayerCommandLineKey = TEXT("-TotalPlayers=");
	const FString TotalNPCsCommandLineKey = TEXT("-TotalNPCs=");
	const FString RequiredPlayersCommandLineKey = TEXT("-RequiredPlayers=");

	const FString NFRFailureString = TEXT("NFR scenario failed");

} // anonymous namespace

FString ABenchmarkGymGameModeBase::ReadFromCommandLineKey = TEXT("ReadFromCommandLine");

ABenchmarkGymGameModeBase::ABenchmarkGymGameModeBase()
	: ExpectedPlayers(1)
	, RequiredPlayers(4096)
	, AveragedClientRTTSeconds(0.0)
	, AveragedClientUpdateTimeDeltaSeconds(0.0)
	, MaxClientRoundTripSeconds(150)
	, MaxClientUpdateTimeDeltaSeconds(300)
	, bHasUxFailed(false)
	, bHasFpsFailed(false)
	, bHasDonePlayerCheck(false)
	, bHasClientFpsFailed(false)
	, bHasActorCountFailed(false)
	, bActorCountFailureState(false)
	, bExpectedActorCountsInitialised(false)
	, ActivePlayers(0)
	, LastTimeHandOverActors(0)
	, TotalHandOverdActorsOfCurrentWorker(0)
	, HandOverFrameCount(0)
	, HandOverTotalFrameCount(1800)
	, PrintMetricsTimer(10)
	, TestLifetimeTimer(0)
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
					Delegate.BindUObject(this, &ABenchmarkGymGameModeBase::GetTotalHandOverActors);
					SpatialMetrics->SetCustomMetric(HandOverActorsMetricName, Delegate);
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
	TickActorHandOver(DeltaSeconds);
	
	// PrintMetricsTimer needs to be reset at the the end of ABenchmarkGymGameModeBase::Tick.
	// This is so that the above function have a chance to run logic dependant on PrintMetricsTimer.HasTimerGoneOff().
	if (HasAuthority() &&
		PrintMetricsTimer.HasTimerGoneOff())
	{
		PrintMetricsTimer.SetTimer(10);
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
	if (Constants->PlayerCheckMetricDelay.HasTimerGoneOff())
	{
		bHasDonePlayerCheck = true;
		if (ActivePlayers < RequiredPlayers)
		{
			// This log is used by the NFR pipeline to indicate if a client failed to connect
			NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: Client connection dropped. Required %d, got %d, num players %d"), *NFRFailureString, RequiredPlayers, ActivePlayers, GetNumPlayers());
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

			if (Component->GetOwner()->HasAuthority())
			{
				UXComponentCount++;
			}
		}
	}
	ReportAuthoritativePlayers(FPlatformProcess::ComputerName(), UXComponentCount);

	if (UXComponentCount == 0 || !HasAuthority())
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
		Constants->UXMetricDelay.HasTimerGoneOff())
	{
		bHasUxFailed = true;
		NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: UX metric check. RTT: %.8f, UpdateDelta: %.8f, ActivePlayers: %d"), *NFRFailureString, AveragedClientRTTSeconds, AveragedClientUpdateTimeDeltaSeconds, ActivePlayers);
	}

	if (PrintMetricsTimer.HasTimerGoneOff())
	{
		NFR_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("UX metric values. RTT: %.8f(%d), UpdateDelta: %.8f(%d), ActivePlayers: %d"), AveragedClientRTTSeconds, ValidRTTCount, AveragedClientUpdateTimeDeltaSeconds, ValidUpdateTimeDeltaCount, ActivePlayers);
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

		FParse::Value(*CommandLine, *MaxRoundTripCommandLineKey, MaxClientRoundTripSeconds);
		FParse::Value(*CommandLine, *MaxUpdateTimeDeltaCommandLineKey, MaxClientUpdateTimeDeltaSeconds);
	}
	else if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString ExpectedPlayersString, RequiredPlayersString, TotalNPCsString, MaxRoundTrip, MaxUpdateTimeDelta, LifetimeString;

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

				if (SpatialWorkerFlags->GetWorkerFlag(TestLiftimeWorkerFlag, LifetimeString))
				{
					SetLifetime(FCString::Atoi(*LifetimeString));
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
		MaxClientRoundTripSeconds = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == MaxUpdateTimeDeltaWorkerFlag)
	{
		MaxClientUpdateTimeDeltaSeconds = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == TestLiftimeWorkerFlag)
	{
		SetLifetime(FCString::Atoi(*FlagValue));
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

void ABenchmarkGymGameModeBase::ReportAuthoritativePlayers_Implementation(const FString& WorkerID, int AuthoritativePlayers)
{
	if (HasAuthority())
	{
		int& Value = MapAuthoritativePlayers.FindOrAdd(WorkerID);
		if (Value != AuthoritativePlayers)
		{
			Value = AuthoritativePlayers;
			ActivePlayers = 0;
			for (const auto& kv : MapAuthoritativePlayers)
			{
				ActivePlayers += kv.Value;
			}
		}
	}
}

double ABenchmarkGymGameModeBase::GetTotalAuthoritativePlayers() const
{
	double TotalPlayers = 0;
	for (const auto& kv : MapAuthoritativePlayers)
	{
		TotalPlayers += kv.Value;
	}
	return TotalPlayers;
}

void ABenchmarkGymGameModeBase::TickActorHandOver(float DeltaSeconds)
{
	// Count how many actors hand over authority in 1 tick
	int AuthoritativeActors = GetAuthoritativeActors();
	int Delta = FMath::Abs(AuthoritativeActors - LastTimeHandOverActors);
	ToBeRemovedHandOverActors.Push(Delta);
	if (HandOverFrameCount > HandOverTotalFrameCount)
	{
		int RemoveValue = ToBeRemovedHandOverActors.Pop(true);
		TotalHandOverdActorsOfCurrentWorker -= RemoveValue;
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("RemoveValue=%d"), RemoveValue);
	}
	TotalHandOverdActorsOfCurrentWorker += Delta;
	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Delta=%d,TotalHandOverdActorsOfCurrentWorker=%d,AuthoritativeActors=%d,LastTimeHandOverActors=%d"), 
		Delta, TotalHandOverdActorsOfCurrentWorker, AuthoritativeActors, LastTimeHandOverActors);
	LastTimeHandOverActors = AuthoritativeActors;
	++HandOverFrameCount;

	// Report TotalHandOverdActorsOfCurrentWorker to the worker which has authority
	ReportHandOverActors(FPlatformProcess::ComputerName(), TotalHandOverdActorsOfCurrentWorker);
}

int ABenchmarkGymGameModeBase::GetAuthoritativeActors() const
{
	int TotalAuthoritativeActors = 0;
	for (TObjectIterator<UMovementComponent> Itr; Itr; ++Itr)
	{
		UMovementComponent* Component = *Itr;
		if (Component->GetOwner() != nullptr && Component->GetOwner()->HasAuthority() && Component->GetWorld() == GetWorld())
		{
			TotalAuthoritativeActors += 1;
		}
	}
	return TotalAuthoritativeActors;
}

void ABenchmarkGymGameModeBase::ReportHandOverActors_Implementation(const FString& WorkerID, int HandOverActors)
{
	if (HasAuthority())
	{
		int& Value = MapHandOverActors.FindOrAdd(WorkerID);
		Value = HandOverActors;
	}
}

double ABenchmarkGymGameModeBase::GetTotalHandOverActors() const
{
	double TotalHandOverActors = 0;
	for (const auto& kv : MapHandOverActors)
	{
		TotalHandOverActors += kv.Value;
	}
	UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("TotalHandOverActors=%d"), TotalHandOverActors);
	return TotalHandOverActors;
}
