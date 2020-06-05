// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNFRConstants, Log, All);

class NFRConstants
{
	int64 TimeToStartFPSSampling;
	bool bFPSSamplingValid{false};
	bool bInitialized{ false };
	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;

	NFRConstants() {}
public:
	
	bool SamplesForFPSValid(); // Wait until this is true until checking FPS values
	void Init(UWorld* World);
	static NFRConstants& Get();

	float GetMinServerFPS() const;
	float GetMinClientFPS() const;
};