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

	class FActorCountInfo
	{
	public:

		explicit FActorCountInfo(int32 InTotal)
			: Total(InTotal)
			, SmoothedTotal(InTotal)
		{}

		explicit FActorCountInfo()
			: Total(0)
			, SmoothedTotal(0.0f)
		{}

		void SetTotal(int InTotal)
		{
			Total = InTotal;
			SmoothedTotal = SmoothedTotal * 0.9f + Total * 0.1f;
		}

		int32 GetTotal() const { return Total; }
		int32 GetSmoothedTotal() const { return FMath::RoundToInt(SmoothedTotal); }

	private:
		int32 Total;
		float SmoothedTotal;
	};

	struct FExpectedActorCountConfig
	{
		explicit FExpectedActorCountConfig(int32 InExpectedCount, int32 InVariance)
			: ExpectedCount(InExpectedCount)
			, Variance(InVariance)
		{}

		explicit FExpectedActorCountConfig()
			: ExpectedCount(0)
			, Variance(0)
		{}

		int32 ExpectedCount;
		int32 Variance;
	};

	using ActorCountMap = TMap<TSubclassOf<AActor>, FActorCountInfo>;

public:
	ABenchmarkGymGameModeBase();

protected:

	static FString ReadFromCommandLineKey;

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
	void AddExpectedActorCount(const TSubclassOf<AActor>& ActorClass, int32 ExpectedCount, int32 Variance);

	virtual void ParsePassedValues();

	UFUNCTION()
	virtual void OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue);

	UFUNCTION(BlueprintNativeEvent)
	void OnTotalNPCsUpdated(int32 Value);
	virtual void OnTotalNPCsUpdated_Implementation(int32 Value) {};

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportMigration(const FString& WorkerID, const float Migration);

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportAuthoritativeActorCount(const FString& WorkerID, TSubclassOf<AActor> ActorClass, int32 ActualCount);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	int32 GetNumWorkers() const { return NumWorkers; }
	int32 GetNumSpawnZones() const { return NumSpawnZones; }

private:

	double AveragedClientRTTMS; // The stored average of all the client RTTs
	double AveragedClientUpdateTimeDeltaMS; // The stored average of the client view delta.
	int32 MaxClientRoundTripMS; // Maximum allowed roundtrip
	int32 MaxClientUpdateTimeDeltaMS;
	bool bHasUxFailed;
	bool bHasFpsFailed;
	bool bHasClientFpsFailed;
	bool bHasActorCountFailed;
	// bActorCountFailureState will be true if the test has failed
	bool bActorCountFailureState;
	bool bExpectedActorCountsInitialised;

	// For actor migration count
	bool bHasActorMigrationCheckFailed;
	int32 PreviousTickMigration;
	typedef TTuple<int32, float> MigrationDeltaPair;
	TQueue<MigrationDeltaPair> MigrationDeltaHistory;
	int32 UXAuthActorCount;
	int32 MigrationOfCurrentWorker;
	float MigrationSeconds;
	float MigrationCountSeconds;
	float MigrationWindowSeconds;
	TMap<FString, float> MapWorkerActorMigration;
	float MinActorMigrationPerSecond;
	FMetricTimer ActorMigrationReportTimer;
	FMetricTimer ActorMigrationCheckTimer;
	FMetricTimer ActorMigrationCheckDelay;
	
	FMetricTimer PrintMetricsTimer;
	FMetricTimer TestLifetimeTimer;
	FMetricTimer TickActorCountTimer;

	ActorCountMap TotalActorCounts;
	TMap<FString, ActorCountMap> WorkerActorCounts;
	TMap<TSubclassOf<AActor>, FExpectedActorCountConfig> ExpectedActorCounts;

	// For total player
	bool bHasRequiredPlayersCheckFailed;
	FMetricTimer RequiredPlayerCheckTimer;
	FMetricTimer DeploymentValidTimer;
	
	int32 NumWorkers;
	int32 NumSpawnZones;
#if	STATS
	// For stat profile
	int32 CPUProfileInterval;
	FMetricTimer StatStartFileTimer;
	FMetricTimer StatStopFileTimer;
	//For MemReport profile
	int32 MemReportInterval;
	FMetricTimer MemReportIntervalTimer;
#endif

	void TryInitialiseExpectedActorCounts();

	void TryBindWorkerFlagsDelegate();
	void TryAddSpatialMetrics();

	void TickPlayersConnectedCheck(float DeltaSeconds);
	void TickServerFPSCheck(float DeltaSeconds);
	void TickClientFPSCheck(float DeltaSeconds);
	void TickUXMetricCheck(float DeltaSeconds);
	void TickActorCountCheck(float DeltaSeconds);
	void TickActorMigration(float DeltaSeconds);

	void SetTotalNPCs(int32 Value);

	double GetClientRTT() const { return AveragedClientRTTMS; }
	double GetClientUpdateTimeDelta() const { return AveragedClientUpdateTimeDeltaMS; }
	double GetRequiredPlayersValid() const { return !bHasRequiredPlayersCheckFailed ? 1.0 : 0.0; }
	double GetTotalMigrationValid() const { return !bHasActorMigrationCheckFailed ? 1.0 : 0.0; }
	double GetFPSValid() const { return !bHasFpsFailed ? 1.0 : 0.0; }
	double GetClientFPSValid() const { return !bHasClientFpsFailed ? 1.0 : 0.0; }
	double GetActorCountValid() const { return !bActorCountFailureState ? 1.0 : 0.0; }

	int32 GetActorAuthCount(const TSubclassOf<AActor>& ActorClass) const;

	void SetLifetime(int32 Lifetime);
	int32 GetPlayerControllerCount() const;
#if	STATS
	void InitStatTimer(const FString& CPUProfileString);
	void InitMemReportTimer(const FString& MemReportIntervalString);
#endif

	UFUNCTION()
	void OnRepTotalNPCs();

	void CheckTotalCountsForActors();
};
