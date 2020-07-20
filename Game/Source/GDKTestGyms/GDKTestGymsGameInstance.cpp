// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsGameInstance.h"

#include "Interop/Connection/SpatialConnectionManager.h"

#include "EngineMinimal.h"
#include "GeneralProjectSettings.h"
#include "NFRConstants.h"
#include "SpatialNetDriver.h"
#include "Utils/SpatialMetrics.h"

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
