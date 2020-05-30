// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsGameInstance.h"

#include "Interop/Connection/SpatialConnectionManager.h"

#include "EngineMinimal.h"

void UGDKTestGymsGameInstance::Init()
{
	Super::Init();

	TickDelegate = FTickerDelegate::CreateUObject(this, &UGDKTestGymsGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	TickWindowTotal = 0;
	GetEngine()->NetworkFailureEvent.AddUObject(this, &UGDKTestGymsGameInstance::NetworkFailureEventCallback);
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

	if (FailureType == ENetworkFailure::ConnectionTimeout)
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
#if !WITH_EDITOR // Don't pollute logs in editor
		UE_LOG(LogTemp, Display, TEXT("FramesPerSecond is %f, 2 min avg is %f"), 1.f / DeltaSeconds, AverageFPS);
#endif
	}

	return true;
}
