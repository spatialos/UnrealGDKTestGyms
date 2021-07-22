// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "BenchmarkGymGameMode.h"
#include "UptimeCrossServerBeacon.h"
#include "AUptimeGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUptimeGymGameMode, Log, All);

UCLASS()
class GDKTESTGYMS_API AUptimeGameMode : public ABenchmarkGymGameMode
{
	GENERATED_BODY()
public:
	AUptimeGameMode();

	int32 GetEgressTestSize() const { return EgressSize; }
	int32 GetEgressTestNum() const { return EgressFrequency; }

protected:

	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Classes)
	TSubclassOf<AUptimeCrossServerBeacon> CrossServerClass;

	virtual void StartCustomNPCSpawning() override;

	virtual void ReadCommandLineArgs(const FString& CommandLine) override;
	virtual void ReadWorkerFlagValues(USpatialWorkerFlags* SpatialWorkerFlags) override;
	virtual void BindWorkerFlagDelegates(USpatialWorkerFlags* SpatialWorkerFlags) override;

private:

	int32 EgressSize;
	int32 EgressFrequency;

	int32 CrossServerSize;
	int32 CrossServerFrequency;

	void SpawnCrossServerActors();

	// Worker flag update delegate functions
	UFUNCTION()
	void OnEgressSizeFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnEgressFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnCrossServerSizeFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnCrossServerFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue);
};
