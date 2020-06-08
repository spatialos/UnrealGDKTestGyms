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
	
	void InitWithWorld(UWorld* World);

	bool SamplesForFPSValid() const; // Wait until this is true until checking FPS values

	float GetMinServerFPS() const;
	float GetMinClientFPS() const;
	
	static const UNFRConstants* Get(UWorld* World);
private:
	int64 TimeToStartFPSSampling;
	mutable bool bFPSSamplingValid{ false };
	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;

	UNFRConstants();
};