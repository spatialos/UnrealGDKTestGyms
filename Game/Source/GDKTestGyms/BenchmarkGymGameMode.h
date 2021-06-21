// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"
#include "BenchmarkGymNPCSpawner.h"
#include "BenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameMode, Log, All);

using ControllerIntegerPair = TPair<TWeakObjectPtr<AController>, int>;

UCLASS()
class GDKTESTGYMS_API USpawnArea : public UObject
{
	GENERATED_BODY()
public:

	enum Type : uint8
	{
		Zone,
		Boundary
	};

	Type Type;
	FVector WorldPosition;
	float Width;
	float Height;
	int32 MaxSpawnPoints;
	int32 MaxSupportedPlayersPerSpawnPoint;
	float MinDistanceBetweenSpawnPoints;

	int32 GetTotalSupportedPlayers() const;
	void GenerateSpawnPointsActors();
	AActor* GetSpawnPointActorByIndex(const int32 Index) const;

private:

	UPROPERTY()
	TArray<AActor*> SpawnPointActors;
};

UCLASS()
class GDKTESTGYMS_API USpawnManager: public UObject
{
	GENERATED_BODY()
public:

	// Will create SpawnAreas for zones and boundaries for given parameters and add them to the member SpawnAreas.
	// Each SpawnArea will be set up to only create a certain number of spawn points.
	// This function should allow you to vary how many NPCs/Simplayers are spawned in the centre of zones or on boundaries.

	void GenerateSpawnPoints(const int32 ZoneRows, const int32 ZoningCols, const int32 ZoneWidth, const int32 ZoneHeight,
		const int32 ZoneSpawnPoints, const int32 BoundarySpawnPoints,
		const int32 MaxSupportedPlayersPerSpawnPoint, const int32 MinDistanceBetweenSpawnPoints);

	AActor* GetSpawnPointActorByIndex(const int32 Index) const;
	int32 GetTotalSupportedPlayers() const;

	void ClearSpawnPoints();

	void ForEachZoneArea(TFunctionRef<void(USpawnArea& ZoneArea)> Predicate);

private:

	int32 TotalSupportedPlayers;

	UPROPERTY()
	TArray<USpawnArea*> SpawnAreas;
};

UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameMode : public ABenchmarkGymGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameMode();
	AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	virtual void BeginPlay() override;

protected:

	UPROPERTY()
	USpawnManager* SpawnManager;

	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Classes)
	TSubclassOf<AActor> DropCubeClass;

	int DistBetweenClusterCenters;
	float PercentageSpawnPointsOnWorkerBoundaries;

	virtual void Tick(float DeltaSeconds) override;

	virtual void StartCustomNPCSpawning();
	virtual void BuildExpectedActorCounts() override;

	virtual void ReadCommandLineArgs(const FString& CommandLine) override;
	virtual void ReadWorkerFlagValues(USpatialWorkerFlags* SpatialWorkerFlags) override;
	virtual void BindWorkerFlagDelegates(USpatialWorkerFlags* SpatialWorkerFlags) override;

	virtual void AddSpatialMetrics(USpatialMetrics* SpatialMetrics) override;

	virtual void OnTotalNPCsUpdated_Implementation(int32 Value) override;

private:

	TArray<FBlackboardValues> PlayerRunPoints;
	TArray<FBlackboardValues> NPCRunPoints;

	TArray<ControllerIntegerPair> AIControlledPlayers;

	bool bHasCreatedSpawnPoints;

	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	int32 PlayersSpawned;
	int32 NPCSToSpawn;

	// Actor migration members
	bool bHasActorMigrationCheckFailed;
	int32 PreviousTickMigration;
	typedef TTuple<int32, float> MigrationDeltaPair;
	TQueue<MigrationDeltaPair> MigrationDeltaHistory;
	int32 MigrationOfCurrentWorker;
	float MigrationSeconds;
	float MigrationCountSeconds;
	float MigrationWindowSeconds;
	TMap<FString, float> MapWorkerActorMigration;
	float MinActorMigrationPerSecond;
	FMetricTimer ActorMigrationReportTimer;
	FMetricTimer ActorMigrationCheckTimer;
	FMetricTimer ActorMigrationCheckDelay;

	UPROPERTY()
	TMap<int32, AActor*> PlayerIdToSpawnPointMap;

	UPROPERTY()
	ABenchmarkGymNPCSpawner* NPCSpawner;

	void GenerateTestScenarioLocations();
	void ClearExistingSpawnPoints();
	void TryStartCustomNPCSpawning();
	void GenerateSpawnPoints();

	void TickActorMigration(float DeltaSeconds);
	void TickNPCSpawning();
	void TickSimPlayerBlackboardValues();

	void SpawnNPCs(int NumNPCs);
	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);

	double GetTotalMigrationValid() const { return !bHasActorMigrationCheckFailed ? 1.0 : 0.0; }

	UFUNCTION(CrossServer, Reliable)
	virtual void ReportMigration(const FString& WorkerID, const float Migration);

	// Worker flag update delegate functions
	UFUNCTION()
	void OnPlayerDensityFlagUpdate(const FString& FlagName, const FString& FlagValue);
};
