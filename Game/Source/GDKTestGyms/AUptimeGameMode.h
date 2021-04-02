// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"
#include "BenchmarkGymNPCSpawner.h"
#include "AUptimeGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUptimeGymGameMode, Log, All);

typedef TPair<TWeakObjectPtr<AController>, int> ControllerIntegerPair;

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API AUptimeGameMode : public ABenchmarkGymGameModeBase
{
	GENERATED_BODY()
public:
	AUptimeGameMode();
	AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	int32 GetEgressTestSize()const { return TestDataSize; }
	int32 GetEgressTestNum() const { return TestDataFrequency; }

protected:
	virtual void BuildExpectedActorCounts() override;
	virtual void OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue) override;

private:
	TArray<FBlackboardValues> PlayerRunPoints;
	TArray<FBlackboardValues> NPCRunPoints;
	void GenerateTestScenarioLocations();

	TArray<ControllerIntegerPair> AIControlledPlayers;

	virtual void Tick(float DeltaSeconds) override;
	virtual void ParsePassedValues() override;

	// Custom density spawning parameters.
	bool bInitializedCustomSpawnParameters;

	// Number of Spawn Cols and Row need to configuration through worker flags
	int32 SpawnCols;
	int32 SpawnRows;

	// Width and Height of Spawn Zones
	float ZoneWidth;
	float ZoneHeight;

	int32 TestDataSize;
	int32 TestDataFrequency;

	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	int32 NumPlayerClusters;
	int32 PlayersSpawned;
	UPROPERTY()
		TArray<AActor*> SpawnPoints;
	UPROPERTY()
		TMap<int32, AActor*> PlayerIdToSpawnPointMap;
	TSubclassOf<AActor> DropCubeClass;
	int32 NPCSToSpawn;

	UPROPERTY()
		ABenchmarkGymNPCSpawner* NPCSpawner;

	void CheckCmdLineParameters();
	void ClearExistingSpawnPoints();
	void StartCustomNPCSpawning();
	void SpawnNPCs(int NumNPCs);
	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);
	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	static void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
};
