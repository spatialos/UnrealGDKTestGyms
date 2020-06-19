// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UserExperienceReporter.h"

#include "BenchmarkGymGameModeBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameModeBase, Log, All);

UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameModeBase();

protected:

	static FString ReadFromCommandLineKey;

	// Total number of players that will connect.
	int32 ExpectedPlayers;
	int32 RequiredPlayers;

	// Replicated so that offloading and zoning servers can get updates.
	UPROPERTY(ReplicatedUsing = OnRepTotalNPCs, BlueprintReadWrite)
	int32 TotalNPCs;

	virtual void ParsePassedValues();

	UFUNCTION()
	virtual void OnWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue);

	UFUNCTION(BlueprintNativeEvent)
	void OnTotalNPCsUpdated(int32 Value);
	virtual void OnTotalNPCsUpdated_Implementation(int32 Value) {};

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

private:

	// Test scenarios
	float PrintUXMetricTimer;
	double AveragedClientRTTSeconds; // The stored average of all the client RTTs
	double AveragedClientUpdateTimeDeltaSeconds; // The stored average of the client view delta.
	int32 MaxClientRoundTripSeconds; // Maximum allowed roundtrip
	int32 MaxClientUpdateTimeDeltaSeconds;
	bool bHasUxFailed;
	bool bHasFpsFailed;
	bool bHasDonePlayerCheck;
	bool bHasClientFpsFailed;
	int32 ActivePlayers; // A count of visible UX components

	virtual void BeginPlay() override;

	void TryBindWorkerFlagsDelegate();
	void TryAddSpatialMetrics();

	void TickPlayersConnectedCheck(float DeltaSeconds);
	void TickServerFPSCheck(float DeltaSeconds);
	void TickClientFPSCheck(float DeltaSeconds);
	void TickUXMetricCheck(float DeltaSeconds);

	void SetTotalNPCs(int32 Value);

	double GetClientRTT() const { return AveragedClientRTTSeconds; }
	double GetClientUpdateTimeDelta() const { return AveragedClientUpdateTimeDeltaSeconds; }
	double GetPlayersConnected() const { return ActivePlayers; }
	double GetFPSValid() const { return !bHasFpsFailed ? 1.0 : 0.0; }
	double GetClientFPSValid() const { return !bHasClientFpsFailed ? 1.0 : 0.0; }

	UFUNCTION()
	void OnRepTotalNPCs();
};
