// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "NFRTestConfiguration.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNFRTestSettings, Log, All);

UCLASS(config = NFRTestConfiguration, defaultconfig)
class UNFRTestConfiguration : public UObject
{
	GENERATED_BODY()

public:
	UNFRTestConfiguration(const FObjectInitializer& ObjectInitializer)
	{
		MaxRoundTrip = 90;
		MinClientUpdates = 5;
	}

	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	int MaxRoundTrip; // 80th pct average of window above threshold, ie above 200 ms round trip

	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	int MinClientUpdates; // 80pct average of client update frequency below threshold ie 20 updates / sec about player

	//UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	//float MinWorldUpdates;
};