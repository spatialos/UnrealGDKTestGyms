// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"
#include "BenchmarkGymNPCSpawner.h"
#include "BenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameMode, Log, All);

typedef TPair<TWeakObjectPtr<AController>, int> ControllerIntegerPair;

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameMode : public ABenchmarkGymGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameMode();

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
private:
	TArray<FBlackboardValues> PlayerRunPoints;
	TArray<FBlackboardValues> NPCRunPoints;
	void GenerateTestScenarioLocations();

	TArray<ControllerIntegerPair> AIControlledPlayers;

	virtual void Tick(float DeltaSeconds) override;
	virtual void ParsePassedValues() override;

	// Custom density spawning parameters.
	bool bInitializedCustomSpawnParameters;
	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	int32 NumPlayerClusters;
	int32 PlayersSpawned;
	TArray<AActor*> SpawnPoints;
	TMap<int32, AActor*> PlayerIdToSpawnPointMap;
	int32 NPCSToSpawn;

	UPROPERTY()
	ABenchmarkGymNPCSpawner* NPCSpawner;

	bool ShouldUseCustomSpawning();
	void CheckCmdLineParameters();
	void ClearExistingSpawnPoints();
	void StartCustomNPCSpawning();
	void SpawnNPCs(int NumNPCs);
	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);
	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	static void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& NumRows, int& NumCols, int& MinRelativeX, int& MinRelativeY);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
};
