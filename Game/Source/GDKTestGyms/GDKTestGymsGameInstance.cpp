// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsGameInstance.h"

#include "Analytics.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "Interop/Connection/SpatialConnectionManager.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "NFRConstants.h"
#include "Utils/SpatialMetrics.h"

#include "Algo/Copy.h"
#include "Engine/Engine.h"
#include "EngineMinimal.h"
#include "EngineUtils.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkingSetSubsystem.h"
#include "LatencyTracer.h"

static void ProcessLockingCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& OutputDevice)
{
	if (Args.Num() < 2)
	{
		OutputDevice.Log(TEXT("Not enough arguments"));
		return;
	}

	if (World->GetNetMode() == ENetMode::NM_Client)
	{
		Cast<AGDKPlayerController>(World->GetFirstPlayerController())->CallServerLockingCommand(Args);
		return;
	}

	const FString Command = Args[0];

	TArray<AWorkingSetsActor*> Cubes;
	Algo::Copy(TActorRange<AWorkingSetsActor>(World), Cubes);
	Cubes.Sort([](const AWorkingSetsActor& Lhs, const AWorkingSetsActor& Rhs) {
		return Lhs.GetActorLocation().Y < Rhs.GetActorLocation().Y;
	});

	auto GetActors = [&Args, &Cubes](const int32 IndicesStart) {
		TArray<int> CubeIndices;
		Algo::Transform(TArrayView<const FString>(Args.GetData() + IndicesStart, Args.Num() - IndicesStart), CubeIndices,
						[](const FString& Str) {
							return FCString::Atoi(*Str);
						});

		TArray<AActor*> Actors;
		for (const int CubeIndex : CubeIndices)
		{
			Actors.Emplace(Cubes[CubeIndex]);
		}
		return Actors;
	};

	auto* LockingPolicy = Cast<USpatialNetDriver>(World->GetNetDriver())->LockingPolicy;
	if (Command == TEXT("LOCK"))
	{
		auto LockToken = LockingPolicy->AcquireLock(GetActors(1)[0]);
		OutputDevice.Logf(TEXT("Locked with %lld"), LockToken);
	}
	if (Command == TEXT("RELEASE"))
	{
		const int64 LockToken = FCString::Atoi64(*Args[1]);
		static_assert(TAreTypesEqual<TRemoveConst<decltype(LockToken)>::Type(), TRemoveConst<ActorLockToken>::Type()>::Value,
					  "LockToken should be read successfully");
		OutputDevice.Logf(TEXT("Unlocking with %lld"), LockToken);
		const bool bHasUnlocked = LockingPolicy->ReleaseLock(LockToken);
		if (bHasUnlocked)
		{
			OutputDevice.Logf(TEXT("Success!"));
		}
		else
		{
			OutputDevice.Logf(ELogVerbosity::Warning, TEXT("Failure!"));
		}
	}
}

static void ProcessWorkingSetCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& OutputDevice)
{
	if (Args.Num() < 2)
	{
		OutputDevice.Log(TEXT("Not enough arguments"));
		return;
	}

	if (World->GetNetMode() == ENetMode::NM_Client)
	{
		Cast<AGDKPlayerController>(World->GetFirstPlayerController())->CallServerWorkingSetCommand(Args);
		return;
	}

	const FString Command = Args[0];

	USpatialActorSetSubsystem* ActorSetSubsystem = World->GetGameInstance()->GetSubsystem<USpatialActorSetSubsystem>();

	TArray<AWorkingSetsActor*> Cubes;
	Algo::Copy(TActorRange<AWorkingSetsActor>(World), Cubes);
	Cubes.Sort([](const AWorkingSetsActor& Lhs, const AWorkingSetsActor& Rhs) {
		return Lhs.GetActorLocation().Y < Rhs.GetActorLocation().Y;
	});

	auto GetActors = [&Args, &Cubes](const int32 IndicesStart) {
		TArray<int> CubeIndices;
		Algo::Transform(TArrayView<const FString>(Args.GetData() + IndicesStart, Args.Num() - IndicesStart), CubeIndices,
						[](const FString& Str) {
							return FCString::Atoi(*Str);
						});

		TArray<AActor*> Actors;
		for (const int CubeIndex : CubeIndices)
		{
			Actors.Emplace(Cubes[CubeIndex]);
		}
		return Actors;
	};

	if (Command == TEXT("ADD"))
	{
		auto Actors = GetActors(1);
		ActorSetSubsystem->CreateActorSet(Actors[0], Actors, World);
		return;
	}
	if (Command == TEXT("MODIFY"))
	{
		const Worker_EntityId_Key WorkingSetEntityId = FCString::Atoi(*Args[1]);

		auto Actors = GetActors(2);

		ActorSetSubsystem->ModifyActorSet({ WorkingSetEntityId }, Actors[0], Actors, World);

		return;
	}
	if (Command == TEXT("DESTROY"))
	{
		const Worker_EntityId_Key WorkingSetEntityId = FCString::Atoi(*Args[1]);

		ActorSetSubsystem->DisbandActorSet({ WorkingSetEntityId }, World);

		return;
	}
}

FAutoConsoleCommandWithWorldArgsAndOutputDevice WorkingSetDebugCommand(
	TEXT("Spatial.Test.WorkingSets"), TEXT(""),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ProcessWorkingSetCommand));
FAutoConsoleCommandWithWorldArgsAndOutputDevice LockingDebugCommand(
	TEXT("Spatial.Test.Locking"), TEXT(""), FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ProcessLockingCommand));

void AGDKPlayerController::CallServerWorkingSetCommand_Implementation(const TArray<FString>& Args)
{
	ProcessWorkingSetCommand(Args, GetWorld(), *GLog);
}

void AGDKPlayerController::CallServerLockingCommand_Implementation(const TArray<FString>& Args)
{
	ProcessLockingCommand(Args, GetWorld(), *GLog);
}

int AGDKPlayerController::GetPieIndex(UObject* Context)
{
	return GEngine->GetWorldContextFromWorld(Context->GetWorld())->PIEInstance;
}

void UGDKTestGymsGameInstance::Init()
{
	Super::Init();

	TickDelegate = FTickerDelegate::CreateUObject(this, &UGDKTestGymsGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	TickWindowTotal = 0;
	GetEngine()->NetworkFailureEvent.AddUObject(this, &UGDKTestGymsGameInstance::NetworkFailureEventCallback);
	NFRConstants = NewObject<UNFRConstants>(this);
	NFRConstants->InitWithWorld(GetWorld());

	OnSpatialConnected.AddUniqueDynamic(this, &UGDKTestGymsGameInstance::SpatialConnected);

	// init metric provider & record start metric
	TSharedPtr<IAnalyticsProvider> Provider = FAnalytics::Get().GetDefaultConfiguredProvider();
	if (Provider.IsValid())
	{
		Provider->RecordEvent(TEXT("MetricsServiceEditorLoaded"), TArray<FAnalyticsEventAttribute>());
		Provider->StartSession();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StartSession: Failed to get the default analytics provider. Double check your [Analytics] configuration in your INI"));
	}
}
ULatencyTracer* UGDKTestGymsGameInstance::GetOrCreateLatencyTracer()
{
	if (Tracer == nullptr)
	{
		Tracer = NewObject<ULatencyTracer>(this);
		Tracer->InitTracer();
	}
	return Tracer;
}

void UGDKTestGymsGameInstance::Shutdown()
{
	// record shutdown metric
	TSharedPtr<IAnalyticsProvider> Provider = FAnalytics::Get().GetDefaultConfiguredProvider();
	if (Provider.IsValid())
	{
		Provider->RecordEvent(TEXT("MetricsServiceEditorClosed"), TArray<FAnalyticsEventAttribute>());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StopSession: Failed to get the default analytics provider. Double check your [Analytics] configuration in your INI"));
	}
	
	Super::Shutdown();
}


void UGDKTestGymsGameInstance::SpatialConnected()
{
	UNetDriver * NetDriver = GEngine->FindNamedNetDriver(GetWorld(), NAME_PendingNetDriver);
	USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(NetDriver);
	if (SpatialNetDriver)
	{
		SpatialNetDriver->SpatialMetrics->WorkerMetricsUpdated.AddLambda([](const USpatialMetrics::WorkerGaugeMetric& GauageMetrics, const USpatialMetrics::WorkerHistogramMetrics& HistogramMetrics)
			{
				for (const TPair<FString, USpatialMetrics::WorkerHistogramValues>& Metric : HistogramMetrics)
				{
					if (Metric.Key == TEXT("kcp_resends_by_packet") && Metric.Value.Sum > 0.0)
					{
						for (const auto& Bucket : Metric.Value.Buckets)
						{
							UE_LOG(LogTemp, Log, TEXT("kcp_resends_by_packet bucket [%.8f] %d"), Bucket.Key, Bucket.Value);
						}
					}
				}
			});
	}	
}

void UGDKTestGymsGameInstance::OnStart()
{
	ENetMode NetMode = GetWorld()->GetNetMode();
	if (NetMode == NM_Client || NetMode == NM_Standalone)
	{
		GetEngine()->SetMaxFPS(60.0f);
	}
}

float UGDKTestGymsGameInstance::AddAndCalcFps(int64 NowReal, float DeltaS)
{
	int64 DeltaTicks = FTimespan::FromSeconds(DeltaS).GetTicks();
	TicksForFPS.Push(TPair<int64, int64>(NowReal, DeltaTicks)); 
	TickWindowTotal += DeltaTicks;

	int64 MinRange = NowReal - FTimespan::FromMinutes(2.0).GetTicks();
	
	// Trim samples
	int NumToRemove = 0;
	for(int i = 0; i < TicksForFPS.Num(); i++)
	{
		const FPSTimePoint& TimePoint = TicksForFPS[i];
		if (TimePoint.Key > MinRange) // Find the first valid sample
		{
			NumToRemove = i;
			break;
		}
	}
	if (NumToRemove > 0)
	{
		for (int i = 0; i < NumToRemove; i++)
		{
			TickWindowTotal -= TicksForFPS[i].Value;
		}
		memmove(&TicksForFPS[0], &TicksForFPS[NumToRemove], (TicksForFPS.Num() - NumToRemove) * sizeof(FPSTimePoint));
		TicksForFPS.SetNum(TicksForFPS.Num() - NumToRemove);
	}
	if (TicksForFPS.Num() > 0)
	{
		return static_cast<float>(TicksForFPS.Num()) / FTimespan(TickWindowTotal).GetTotalSeconds(); // 1.0f / Total / Samples 
	}
	return 60.0f; // If there are no samples return the ideal
}

void UGDKTestGymsGameInstance::NetworkFailureEventCallback(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogTemp, Warning, TEXT("UGDKTestGymsGameInstance: Network Failure (%s)"), *ErrorString);

	if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking() && FailureType == ENetworkFailure::ConnectionTimeout)
	{
		if (USpatialConnectionManager* ConnectionManager = GetSpatialConnectionManager())
		{
			UE_LOG(LogTemp, Warning, TEXT("UGDKTestGymsGameInstance: Retrying connection..."));
			bool bConnectAsClient = (GetWorld()->GetNetMode() == NM_Client);
			ConnectionManager->Connect(bConnectAsClient, 0 /*PlayInEditorID*/);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UGDKTestGymsGameInstance: Connection manager invalid, won't retry connection."));
		}
	}
}

bool UGDKTestGymsGameInstance::Tick(float DeltaSeconds)
{	
	AverageFPS = AddAndCalcFps(FDateTime::Now().GetTicks(), DeltaSeconds);
	SecondsSinceFPSLog += DeltaSeconds;

	if (SecondsSinceFPSLog > 10.0f) 
	{
		SecondsSinceFPSLog = 0.0f;
		NFR_LOG(LogTemp, Log, TEXT("FramesPerSecond is %f, 2 min avg is %f"), 1.f / DeltaSeconds, AverageFPS);
	}

	return true;
}
