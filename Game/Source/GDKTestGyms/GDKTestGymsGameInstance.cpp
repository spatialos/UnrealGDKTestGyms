// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsGameInstance.h"

#include "EngineMinimal.h"

void UGDKTestGymsGameInstance::Init()
{
	Super::Init();

	TickDelegate = FTickerDelegate::CreateUObject(this, &UGDKTestGymsGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	TickWindowTotal = 0;
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
			int NumToRemove = i;
			break;
		}
	}
	if (NumToRemove > 0)
	{
		for (int j = 0; j < NumToRemove; j++)
		{
			TickWindowTotal -= TicksForFPS[j].Value;
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
