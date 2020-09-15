// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UserExperienceReporter.h"
#include "NFRConstants.h"

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

	struct FExpectedActorCount
	{
		explicit FExpectedActorCount(TSubclassOf<AActor> InActorClass, int32 InExpectedCount, int32 InVariance)
			: ActorClass(InActorClass)
			, ExpectedCount(InExpectedCount)
			, Variance(InVariance)
		{}

		TSubclassOf<AActor> ActorClass;
		int32 ExpectedCount;
		int32 Variance;
	};

	// Total number of players that will attempt to connect.
	int32 ExpectedPlayers;

	// Total number of players that must connect for the test to pass. Must be less than ExpectedPlayers.
	// This allows some accepted client flakes without failing the overall test.
	int32 RequiredPlayers;

	// Replicated so that offloading and zoning servers can get updates.
	UPROPERTY(ReplicatedUsing = OnRepTotalNPCs, BlueprintReadWrite)
	int32 TotalNPCs;

	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Classes)
	TSubclassOf<APawn> NPCClass;

	virtual void BuildExpectedActorCounts();
	void AddExpectedActorCount(TSubclassOf<AActor> ActorClass, int32 ExpectedCount, int32 Variance);

	int32 GetActorClassCount(TSubclassOf<AActor> ActorClass) const;

	virtual void ParsePassedValues();

	UFUNCTION()
	virtual void OnWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue);

	UFUNCTION(BlueprintNativeEvent)
	void OnTotalNPCsUpdated(int32 Value);
	virtual void OnTotalNPCsUpdated_Implementation(int32 Value) {};

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportAuthoritativePlayers(const FString& WorkerID, int AuthoritativePlayers);
	void ReportAuthoritativePlayers_Implementation(const FString& WorkerID, int AuthoritativePlayers);

private:
	// Test scenarios

	double AveragedClientRTTSeconds; // The stored average of all the client RTTs
	double AveragedClientUpdateTimeDeltaSeconds; // The stored average of the client view delta.
	int32 MaxClientRoundTripSeconds; // Maximum allowed roundtrip
	int32 MaxClientUpdateTimeDeltaSeconds;
	bool bHasUxFailed;
	bool bHasFpsFailed;
	bool bHasDonePlayerCheck;
	bool bHasClientFpsFailed;
	bool bHasActorCountFailed;
	// bActorCountFailureState will be true if the test has failed
	bool bActorCountFailureState;
	bool bExpectedActorCountsInitialised;
	int32 ActivePlayers; // All authoritative players from all workers

	FMetricTimer PrintMetricsTimer;
	FMetricTimer TestLifetimeTimer;

	TArray<FExpectedActorCount> ExpectedActorCounts;
	TMap<FString, int>	MapAuthoritativePlayers;

	virtual void BeginPlay() override;

	void TryInitialiseExpectedActorCounts();

	void TryBindWorkerFlagsDelegate();
	void TryAddSpatialMetrics();

	void TickPlayersConnectedCheck(float DeltaSeconds);
	void TickServerFPSCheck(float DeltaSeconds);
	void TickClientFPSCheck(float DeltaSeconds);
	void TickUXMetricCheck(float DeltaSeconds);
	void TickActorCountCheck(float DeltaSeconds);

	void SetTotalNPCs(int32 Value);

	double GetClientRTT() const { return AveragedClientRTTSeconds; }
	double GetClientUpdateTimeDelta() const { return AveragedClientUpdateTimeDeltaSeconds; }
	double GetPlayersConnected() const { return static_cast<double>(ActivePlayers); }
	double GetFPSValid() const { return !bHasFpsFailed ? 1.0 : 0.0; }
	double GetClientFPSValid() const { return !bHasClientFpsFailed ? 1.0 : 0.0; }
	double GetActorCountValid() const { return !bActorCountFailureState ? 1.0 : 0.0; }

	void SetLifetime(int32 Lifetime);

	UFUNCTION()
	void OnRepTotalNPCs();
};
