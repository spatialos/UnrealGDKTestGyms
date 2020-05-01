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
		MaxRoundTrip = 60;
		MinClientUpdates = 20;
	}

	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	int MaxRoundTrip;

	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	int MinClientUpdates;

	//UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	//float MinWorldUpdates;
};