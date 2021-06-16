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
	virtual void ReadWorkerFlagsValues(USpatialWorkerFlags* SpatialWorkerFlags) override;
	virtual void BindWorkerFlagsDelegates(USpatialWorkerFlags* SpatialWorkerFlags) override;

private:

	int32 EgressSize;
	int32 EgressFrequency;

	int32 CrossServerSize;
	int32 CrossServerFrequency;

	void SpawnCrossServerActors(int32 CrossServerPoint);
	TArray<FVector> GenerateCrossServerLoaction();

	// Worker flag update delegate functions
	UFUNCTION()
	void OnUptimeEgressSizeFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnUptimeEgressFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnUptimeCrossServerSizeFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnUptimeCrossServerFrequencyFlagUpdate(const FString& FlagName, const FString& FlagValue);
};
