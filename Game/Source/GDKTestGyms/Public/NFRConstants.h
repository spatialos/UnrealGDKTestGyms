// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "NFRConstants.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNFRConstants, Log, All);

class FMetricTimer
{
public:

	FMetricTimer() = default;
	FMetricTimer(int32 InTimeToStart);

	bool SetTimer(int32 Seconds);
	void SetLock(bool bState);

	bool HasTimerGoneOff() const;
	int32 GetSecondsRemaining() const;

private:

	bool bLocked;
	int64 TimeToStart;
};

UCLASS()
class UNFRConstants : public UObject
{
public:	

	GENERATED_BODY()

	UNFRConstants();
	
	void InitWithWorld(const UWorld* World);

	float GetMinServerFPS() const;
	float GetMinClientFPS() const;

	float GetMinPlayerAvgVelocity() const;
	
	static const UNFRConstants* Get(const UWorld* World);

	FMetricTimer ServerFPSMetricDelay;
	FMetricTimer ClientFPSMetricDelay;
	FMetricTimer UXMetricDelay;
	FMetricTimer ActorCheckDelay;

private:

	bool bIsInitialised{ false };

	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;

	// Default value means don't report this metric to the nfr.
	float MinPlayerAvgVelocity = -1.0f;
};