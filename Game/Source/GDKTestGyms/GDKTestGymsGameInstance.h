// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "SpatialGameInstance.h"
#include "GDKTestGymsGameInstance.generated.h"

UCLASS()
class GDKTESTGYMS_API UGDKTestGymsGameInstance : public USpatialGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	bool Tick(float DeltaSeconds);
	virtual void OnStart() override;

	void NetworkFailureEventCallback(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

private:
	using FPSTimePoint = TPair<int64, int64>; // Real, FrameDelta
	int64 TickWindowTotal;
	TArray<FPSTimePoint> TicksForFPS;
	float AddAndCalcFps(int64 NowReal, float DeltaS);

	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	float AverageFPS = 60.0f;
	float SecondsSinceFPSLog = 1.0f;
};
