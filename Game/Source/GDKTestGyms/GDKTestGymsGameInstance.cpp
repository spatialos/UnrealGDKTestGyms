// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsGameInstance.h"

#include "EngineMinimal.h"

void UGDKTestGymsGameInstance::Init()
{
	Super::Init();

	TickDelegate = FTickerDelegate::CreateUObject(this, &UGDKTestGymsGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
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
	TicksForFPS.Push(TPair<int64, float>(NowReal, DeltaS)); 
	float TotalDelta = 0.0f;
	int NumSamples = 0;
	int64 MinRange = NowReal - FTimespan::FromMinutes(2.0).GetTicks();
	for (int i = TicksForFPS.Num()-1; i >= 0; i--)
	{
		const FPSTimePoint& TimePoint = TicksForFPS[i];
		if (TimePoint.Key < MinRange)
		{
			TicksForFPS.RemoveAt(i);
		}
		else
		{
			TotalDelta += TimePoint.Value;
			NumSamples++;
		}
	}
	if (NumSamples > 0)
	{
		return static_cast<float>(NumSamples) / TotalDelta; // 1.0f / Total / Samples 
	}
	return 60.0f; // If there are no samples return the ideal
}

bool UGDKTestGymsGameInstance::Tick(float DeltaSeconds)
{	
	float FPS = AddAndCalcFps(FDateTime::Now().GetTicks(), DeltaSeconds);
	SecondsSinceFPSLog += DeltaSeconds;

	if (SecondsSinceFPSLog > 1.0f) 
	{
		SecondsSinceFPSLog = 0.0f;
#if !WITH_EDITOR // Don't pollute logs in editor
		UE_LOG(LogTemp, Display, TEXT("FramesPerSecond is %f"), FPS);
#endif
	}

	return true;
}
