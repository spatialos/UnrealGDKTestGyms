// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "NFRConstants.h"
#include "EngineClasses/SpatialGameInstance.h"

#include "CoreMinimal.h"
#include "Engine/NetDriver.h"
#include "Containers/Ticker.h"

#include "GDKTestGymsGameInstance.generated.h"

#define OUTPUT_NFR_SCENARIO_LOGS !WITH_EDITOR

#if OUTPUT_NFR_SCENARIO_LOGS
#define NFR_LOG UE_LOG
#else
#define NFR_LOG(...) 0
#endif

UCLASS()
class GDKTESTGYMS_API UGDKTestGymsGameInstance : public USpatialGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	bool Tick(float DeltaSeconds);
	virtual void OnStart() override;
	float GetAveragedFPS() const { return AverageFPS; }
	UFUNCTION(BlueprintCallable)
	virtual void ReturnToMainMenu() override;
	void NetworkFailureEventCallback(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	const UNFRConstants* GetNFRConstants() const { return NFRConstants; }
private:
	using FPSTimePoint = TPair<int64, int64>; // Real, FrameDelta
	int64 TickWindowTotal;
	TArray<FPSTimePoint> TicksForFPS;
	float AddAndCalcFps(int64 NowReal, float DeltaS);

	UFUNCTION()
	void SpatialConnected();
	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	UPROPERTY()
	UNFRConstants* NFRConstants;

	float AverageFPS = 60.0f;
	float SecondsSinceFPSLog = 10.0f;
};
