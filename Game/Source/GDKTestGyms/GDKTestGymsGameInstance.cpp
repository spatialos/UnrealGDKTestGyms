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

bool UGDKTestGymsGameInstance::Tick(float DeltaSeconds)
{
	float Alpha = 0.8f;
	float CurrFPS = 1.0f / DeltaSeconds; // TODO - we should remove FApp::GetIdleTime() from DeltaSeconds once the NFR regex is fixed
	AverageFPS = (Alpha * AverageFPS) + ((1.0f - Alpha) * CurrFPS);

	SecondsSinceFPSLog += DeltaSeconds;

	if (SecondsSinceFPSLog > 1.0f) 
	{
		SecondsSinceFPSLog = 0.0f;
#if !WITH_EDITOR // Don't pollute logs in editor
		UE_LOG(LogTemp, Display, TEXT("FramesPerSecond is %f"), AverageFPS);
#endif
	}

	return true;
}
