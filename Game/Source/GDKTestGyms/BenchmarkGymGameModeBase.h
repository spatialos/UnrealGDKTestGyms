// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UserExperienceReporter.h"
#include "NFRConstants.h"
#include "BenchmarkGymGameModeBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameModeBase, Log, All);

USTRUCT()
struct FActorCount
{
	GENERATED_BODY()

	explicit FActorCount() {}
	explicit FActorCount(const TSubclassOf<AActor>& InActorClass, int32 InCount)
		: ActorClass(InActorClass)
		, Count(InCount)
	{}

	UPROPERTY()
	TSubclassOf<AActor> ActorClass;

	UPROPERTY()
	int32 Count;
};

UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	struct FExpectedActorCountConfig
	{
		explicit FExpectedActorCountConfig(int32 InMinCount, int32 InMaxCount)
			: MinCount(InMinCount)
			, MaxCount(InMaxCount)
		{}

		explicit FExpectedActorCountConfig()
			: MinCount(0)
			, MaxCount(0)
		{}

		int32 MinCount;
		int32 MaxCount;
	};

	using ActorCountMap = TMap<TSubclassOf<AActor>, int32>;

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
	void AddExpectedActorCount(const TSubclassOf<AActor>& ActorClass, const int32 MinCount, const int32 MaxCount);

	virtual void ParsePassedValues();

	UFUNCTION()
	virtual void OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue);

	UFUNCTION(BlueprintNativeEvent)
	void OnTotalNPCsUpdated(int32 Value);
	virtual void OnTotalNPCsUpdated_Implementation(int32 Value) {};

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportMigration(const FString& WorkerID, const float Migration);

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportAuthoritativeActorCount(const int32 WorkerActorCountReportIdx, const FString& WorkerID, const TArray<FActorCount>& ActorCounts);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	int32 GetNumWorkers() const { return NumWorkers; }
	int32 GetNumSpawnZones() const { return NumSpawnZones; }

	// For sim player movement metrics
	UFUNCTION(CrossServer, Reliable)
	virtual void ReportAuthoritativePlayerMovement(const FString& WorkerID, const FVector2D& AverageData);

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

	UPROPERTY(ReplicatedUsing = OnActorCountReportIdx)
	int32 ActorCountReportIdx;

	TMap<FString, int32> ActorCountReportedIdxs;

	float TimeSinceLastCheckedTotalActorCounts;
	ActorCountMap TotalActorCounts;
	TMap<FString, ActorCountMap> WorkerActorCounts;
	TMap<TSubclassOf<AActor>, FExpectedActorCountConfig> ExpectedActorCounts;

	// For total player
	bool bHasRequiredPlayersCheckFailed;
	FMetricTimer RequiredPlayerCheckTimer;
	FMetricTimer DeploymentValidTimer;

	// For sim player movement metrics
	TMap<FString, FVector2D> LatestAvgVelocityMap;	// <worker id, <avg, count>>
	float CurrentPlayerAvgVelocity;	// Each report will update this value.
	float RecentPlayerAvgVelocity; // Recent 30 Avg for metrics check
	TArray<float> AvgVelocityHistory;	// Each check will push cur avg value into this queue, and cal avg value.
	FMetricTimer RequiredPlayerMovementReportTimer;
	FMetricTimer RequiredPlayerMovementCheckTimer;

	int32 NumWorkers;
	int32 NumSpawnZones;
#if	STATS
	// For stat profile
	int32 CPUProfileInterval;
	FMetricTimer StatStartFileTimer;
	FMetricTimer StatStopFileTimer;
#endif
#if !UE_BUILD_SHIPPING
	//For MemReport profile
	int32 MemReportInterval;
	FMetricTimer MemReportIntervalTimer;
#endif

	void TryBindWorkerFlagsDelegate();
	void TryAddSpatialMetrics();

	FTimerHandle UpdateActorCountCheckTimerHandle;
	const float UpdateActorCountCheckPeriodInSeconds = 10.0f;
	const float UpdateActorCountCheckInitialDelayInSeconds = 60.0f;
	void InitialiseActorCountCheckTimer();
	void UpdateActorCountCheck();

	void TickPlayersConnectedCheck(float DeltaSeconds);
	void TickPlayersMovementCheck(float DeltaSeconds);
	void TickServerFPSCheck(float DeltaSeconds);
	void TickClientFPSCheck(float DeltaSeconds);
	void TickUXMetricCheck(float DeltaSeconds);
	void TickActorMigration(float DeltaSeconds);



	void SetTotalNPCs(int32 Value);

	double GetClientRTT() const { return AveragedClientRTTMS; }
	double GetClientUpdateTimeDelta() const { return AveragedClientUpdateTimeDeltaMS; }
	double GetRequiredPlayersValid() const { return !bHasRequiredPlayersCheckFailed ? 1.0 : 0.0; }
	double GetTotalMigrationValid() const { return !bHasActorMigrationCheckFailed ? 1.0 : 0.0; }
	double GetFPSValid() const { return !bHasFpsFailed ? 1.0 : 0.0; }
	double GetClientFPSValid() const { return !bHasClientFpsFailed ? 1.0 : 0.0; }
	double GetActorCountValid() const { return !bActorCountFailureState ? 1.0 : 0.0; }
	double GetPlayerMovement() const { return RecentPlayerAvgVelocity; }

	void SetLifetime(int32 Lifetime);
#if	STATS
	void InitStatTimer(const FString& CPUProfileString);
#endif
#if !UE_BUILD_SHIPPING
	void InitMemReportTimer(const FString& MemReportIntervalString);
#endif

	UFUNCTION()
	void OnRepTotalNPCs();

	UFUNCTION()
	void OnActorCountReportIdx();

	void UpdateAndReportActorCounts();
	void UpdateAndCheckTotalActorCounts();
	void GetActorCount(const TSubclassOf<AActor>& ActorClass, int32& OutTotalCount, int32& OutAuthCount) const;

	void GetVelocityForMovementReport();
	void GetPlayersVelocitySum(FVector2D& Velocity);
	void CheckVelocityForPlayerMovement();
};
