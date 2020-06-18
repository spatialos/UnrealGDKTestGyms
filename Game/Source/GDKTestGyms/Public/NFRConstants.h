// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "NFRConstants.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNFRConstants, Log, All);

class FMetricDelay
{
public:

	FMetricDelay();
	FMetricDelay(int64 InTimeToStart);

	bool IsReady() const;

private:
	int64 TimeToStart;
};

UCLASS()
class UNFRConstants : public UObject
{
public:	

	UNFRConstants();

	GENERATED_BODY()
	
	void InitWithWorld(const UWorld* World);

	float GetMinServerFPS() const;
	float GetMinClientFPS() const;
	
	static const UNFRConstants* Get(const UWorld* World);

	FMetricDelay PlayerCheckMetricDelay;
	FMetricDelay ServerFPSMetricDelay;
	FMetricDelay ClientFPSMetricDelay;
	FMetricDelay UXMetricDelay;

private:

	bool bIsInitialised{ false };

	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;
};