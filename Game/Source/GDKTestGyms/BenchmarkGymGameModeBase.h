// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UserExperienceReporter.h"
#include "NFRConstants.h"
#include "MetricsBlueprintLibrary.h"
#include "BenchmarkGymGameModeBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameModeBase, Log, All);

class USpatialWorkerFlags;
class USpatialMetrics;

USTRUCT()
struct FActorCount
{
	GENERATED_BODY()

	explicit FActorCount()
		: ActorClass()
		, Count()
	{}

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Mode")
	bool bLongFormScenario;

	virtual void BuildExpectedActorCounts();
	void AddExpectedActorCount(const TSubclassOf<AActor>& ActorClass, const int32 MinCount, const int32 MaxCount);

	UFUNCTION(BlueprintNativeEvent)
	void OnTotalNPCsUpdated(int32 Value);
	virtual void OnTotalNPCsUpdated_Implementation(int32 Value) {};

	UFUNCTION(BlueprintCallable)
	float GetCubeRespawnBaseTime() const { return CubeRespawnBaseTime; }

	UFUNCTION(BlueprintCallable)
	float GetCubeRespawnRandomRangeTime() const { return CubeRespawnRandomRangeTime; }

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportAuthoritativeActorCount(const int32 WorkerActorCountReportIdx, const FString& WorkerID, const TArray<FActorCount>& ActorCounts);

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void OnAuthorityLost() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	virtual void AddSpatialMetrics(USpatialMetrics* SpatialMetrics);

	virtual void ReadCommandLineArgs(const FString& CommandLine);
	virtual void ReadWorkerFlagValues(USpatialWorkerFlags* SpatialWorkerFlags);
	virtual void BindWorkerFlagDelegates(USpatialWorkerFlags* SpatialWorkerFlags);

	// For sim player movement metrics
	UFUNCTION(CrossServer, Reliable)
	virtual void ReportAuthoritativePlayerMovement(const FString& WorkerID, const FVector2D& AverageData);

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportUserExperience(const FString& WorkerID, float RTTime, float UpdateTime);

	int32 GetNumWorkers() const { return NumWorkers; }
	int32 GetZoningCols() const { return ZoningCols; }
	int32 GetZoningRows() const { return ZoningRows; }
	float GetZoneWidth() const { return ZoneWidth; }
	float GetZoneHeight() const { return ZoneHeight; }
	int32 GetUXAuthActorCount() const { return UXAuthActorCount; }

	const FString NFRFailureString = TEXT("NFR scenario failed");

private:

	int32 NumWorkers;
	int32 ZoningCols;
	int32 ZoningRows;
	float ZoneWidth;
	float ZoneHeight;

	struct UX
	{
		float RTT;
		float UpdateTime;
	};
	TMap<FString, UX> LatestClientUXMap;	// <worker id, UX>
	float AveragedClientRTTMS; // The stored average of all the client RTTs
	float AveragedClientUpdateTimeDeltaMS; // The stored average of the client view delta.
	int32 MaxClientRoundTripMS; // Maximum allowed roundtrip
	int32 MaxClientUpdateTimeDeltaMS;
	bool bHasUxFailed;
	bool bHasFpsFailed;
	bool bHasClientFpsFailed;
	bool bHasActorCountFailed;	// Stores if the actor count check has ever failed.
	bool bActorCountFailureState; // Stores the *current* failure state of the Actor Count checks.
	int32 UXAuthActorCount;

	FMetricTimer PrintMetricsTimer;
	FMetricTimer TestLifetimeTimer;

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
	float CubeRespawnBaseTime;
	float CubeRespawnRandomRangeTime;

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

	void GatherWorkerConfiguration();
	void ParsePassedValues();
	void TryAddSpatialMetrics();
	void TryBindWorkerFlagsDelegates();

	FTimerHandle FailActorCountTimeoutTimerHandle;
	FTimerHandle UpdateActorCountCheckTimerHandle;
	const float UpdateActorCountCheckPeriodInSeconds = 10.0f;
	const float UpdateActorCountCheckInitialDelayInSeconds = 60.0f;
	void InitialiseActorCountCheckTimer();
	void UpdateActorCountCheck();
	void FailActorCountDueToTimeout();

	void TickPlayersConnectedCheck(float DeltaSeconds);
	void TickPlayersMovementCheck(float DeltaSeconds);
	void TickServerFPSCheck(float DeltaSeconds);
	void TickClientFPSCheck(float DeltaSeconds);
	void TickUXMetricCheck(float DeltaSeconds);

	void SetTotalNPCs(int32 Value);

	double GetClientRTT() const { return AveragedClientRTTMS; }
	double GetClientUpdateTimeDelta() const { return AveragedClientUpdateTimeDeltaMS; }
	double GetRequiredPlayersValid() const { return !bHasRequiredPlayersCheckFailed ? 1.0 : 0.0; }
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
	void OutputPlayerDensity();

	// Worker flag update delegate functions
	UFUNCTION()
	void OnExpectedPlayersFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnRequiredPlayersFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnTotalNPCsFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnMaxRoundTripFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnMaxUpdateTimeDeltaFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnTestLiftimeFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnCubeRespawnBaseTimeFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnCubeRespawnRandomRangeTimeUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnStatProfileFlagUpdate(const FString& FlagName, const FString& FlagValue);

	UFUNCTION()
	void OnMemReportFlagUpdate(const FString& FlagName, const FString& FlagValue);
	// Metrics
	typedef double (ABenchmarkGymGameModeBase::* FunctionPtrType)(void) const;
	void GetMetrics(const FString& LeftLabel, const FString& RightLabel, const FString& MetricsName, ABenchmarkGymGameModeBase::FunctionPtrType Func);
};
