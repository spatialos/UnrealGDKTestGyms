// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "NFRConstants.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNFRConstants, Log, All);

UCLASS()
class UNFRConstants : public UObject
{
public:	
	GENERATED_BODY()
	
	void InitWithWorld(const UWorld* World);

	bool PlayerCheckValid() const;
	bool SamplesForServerFPSValid() const; // Wait until this is true until checking Server FPS values
	bool SamplesForClientFPSValid() const; // Wait until this is true until checking Client FPS values
	bool UXMetricValid() const;

	float GetMinServerFPS() const;
	float GetMinClientFPS() const;
	
	static const UNFRConstants* Get(const UWorld* World);
private:
	bool bIsInitialised{ false };
	int64 TimeToStartPlayerCheck;
	int64 TimeToStartServerFPSSampling;
	int64 TimeToStartClientFPSSampling;
	int64 TimeToStartUXMetric;
	mutable bool bPlayerCheckValid{ false };
	mutable bool bServerFPSSamplingValid{ false };
	mutable bool bClientFPSSamplingValid{ false };
	mutable bool bUXMetricValid{ false };
	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;

	bool DoValidityCheck(bool& IsValid, int64 TimeToStart) const;
};