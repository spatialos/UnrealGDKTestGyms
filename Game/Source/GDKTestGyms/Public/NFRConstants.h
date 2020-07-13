// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "NFRConstants.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNFRConstants, Log, All);

class FMetricTimer
{
public:

	FMetricTimer() = default;
	FMetricTimer(int32 InTimeToStart);

	void SetTimer(int32 Seconds);
	bool HasTimerGoneOff() const;


private:
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
	
	static const UNFRConstants* Get(const UWorld* World);

	FMetricTimer PlayerCheckMetricDelay;
	FMetricTimer ServerFPSMetricDelay;
	FMetricTimer ClientFPSMetricDelay;
	FMetricTimer UXMetricDelay;
	FMetricTimer ActorCheckDelay;

private:

	bool bIsInitialised{ false };

	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;
};