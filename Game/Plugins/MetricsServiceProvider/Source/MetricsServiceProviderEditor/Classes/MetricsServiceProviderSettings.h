// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "AnalyticsSettings.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MetricsServiceProviderSettings.generated.h"

UCLASS()
class UMetricsServiceProviderSettings
	: public UAnalyticsSettingsBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString ReleaseApiKey;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString DebugApiKey;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString TestApiKey;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString DevelopmentApiKey;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString ReleaseApiEndpoint;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString DebugApiEndpoint;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString TestApiEndpoint;

	UPROPERTY(EditAnywhere, Category = Metrics, meta = (ConfigRestartRequired = true))
	FString DevelopmentApiEndpoint;

	// UAnalyticsSettingsBase interface
protected:
	/**
	 * Provides a mechanism to read the section based information into this UObject's properties
	 */
	virtual void ReadConfigSettings();
	/**
	 * Provides a mechanism to save this object's properties to the section based ini values
	 */
	virtual void WriteConfigSettings();
};
