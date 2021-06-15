// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"
#include "BenchmarkGymNPCSpawner.h"
#include "BenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameMode, Log, All);

typedef TPair<TWeakObjectPtr<AController>, int> ControllerIntegerPair;

UCLASS()
class GDKTESTGYMS_API USpawnCluster : public UObject
{
	GENERATED_BODY()
public:

	~USpawnCluster();

	FVector WorldPosition;
	float Width;
	float Height;
	int32 MaxSpawnPoints;

	void GenerateSpawnPoints();
	const TArray<AActor*>& CreateSpawnPointActors();

private:

	TArray<FVector> SpawnPoints;

	UPROPERTY()
	TArray<AActor*> SpawnPointActors;
};

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
	int32 MaxClusters;
	int32 MaxSpawnPointsPerCluster;
	float MinDistanceBetweenClusters;

	void GenerateSpawnClusters();
	TArray<AActor*> CreateSpawnPointActors();

private:

	UPROPERTY()
	TArray<USpawnCluster*> SpawnClusters;
};

UCLASS()
class GDKTESTGYMS_API USpawnManager: public UObject
{
	GENERATED_BODY()
public:

	void GenerateSpawnPoints(const int32 ZoneRows, const int32 ZoningCols, const int32 ZoneWidth, const int32 ZoneHeight,
		const int32 ZoneClusters, const int32 BoundaryClusters,
		const int32 MaxSpawnPointsPerCluster, const int32 MinDistanceBetweenClusters);

	AActor* GetSpawnPointActorByIndex(const int32 Index) const;
	int32 GetNumSpawnPoints() const;

	void ClearSpawnPoints();

private:

	void GenerateSpawnAreas(const int32 ZoneRows, const int32 ZoningCols, const int32 ZoneWidth, const int32 ZoneHeight,
		const int32 ZoneClusters, const int32 BoundaryClusters,
		const int32 MaxSpawnPointsPerCluster, const int32 MinDistanceBetweenClusters);
	void CreateSpawnPointActors(int32 ZoneClusters, int32 BoundaryClusters, const int32 MaxSpawnPointsPerCluster);

	UPROPERTY()
	TArray<USpawnArea*> SpawnAreas;

	UPROPERTY()
	TArray<AActor*> SpawnPointActors;
};

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameMode : public ABenchmarkGymGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameMode();
	AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	virtual void BeginPlay() override;

protected:

	virtual void BuildExpectedActorCounts() override;

	virtual void ReadCommandLineArgs(const FString& CommandLine) override;
	virtual void ReadWorkerFlagsValues(USpatialWorkerFlags* SpatialWorkerFlags) override;
	virtual void BindWorkerFlagsDelegates(USpatialWorkerFlags* SpatialWorkerFlags) override;

	virtual void OnExpectedPlayerFlagUpdate(const FString& FlagName, const FString& FlagValue) override;

	UFUNCTION()
	virtual void OnPlayerDensityFlagUpdate(const FString& FlagName, const FString& FlagValue);

private:
	TArray<FBlackboardValues> PlayerRunPoints;
	TArray<FBlackboardValues> NPCRunPoints;
	void GenerateTestScenarioLocations();

	TArray<ControllerIntegerPair> AIControlledPlayers;

	virtual void Tick(float DeltaSeconds) override;

	bool bHasCreatedSpawnPoints;

	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	int32 PlayersSpawned;
	int32 NPCSToSpawn;

	UPROPERTY()
	TMap<int32, AActor*> PlayerIdToSpawnPointMap;
	TSubclassOf<AActor> DropCubeClass;

	UPROPERTY()
	ABenchmarkGymNPCSpawner* NPCSpawner;

	UPROPERTY()
	USpawnManager* SpawnManager;

	void ClearExistingSpawnPoints();
	void TryStartCustomNPCSpawning();
	void GenerateSpawnPoints();
	void SpawnNPCs(int NumNPCs);
	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);
};
