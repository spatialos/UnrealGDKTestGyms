// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "NFRConstants.h"
#include "SpatialGameInstance.h"

#include "GDKTestGymsGameInstance.generated.h"

#define OUTPUT_NFR_SCENARIO_LOGS !WITH_EDITOR
//#define OUTPUT_NFR_SCENARIO_LOGS 1

UCLASS()
class GDKTESTGYMS_API UGDKTestGymsGameInstance : public USpatialGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	bool Tick(float DeltaSeconds);
	virtual void OnStart() override;
	float GetAveragedFPS() const { return AverageFPS; }
	void NetworkFailureEventCallback(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	const UNFRConstants* GetNFRConstants() const { return NFRConstants; }
private:
	using FPSTimePoint = TPair<int64, int64>; // Real, FrameDelta
	int64 TickWindowTotal;
	TArray<FPSTimePoint> TicksForFPS;
	float AddAndCalcFps(int64 NowReal, float DeltaS);

	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	UPROPERTY()
	UNFRConstants* NFRConstants;

	float AverageFPS = 60.0f;
	float SecondsSinceFPSLog = 10.0f;
};
