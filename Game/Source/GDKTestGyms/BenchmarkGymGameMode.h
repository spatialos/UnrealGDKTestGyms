// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGym, Log, All);

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameMode();

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
private:
	bool bHasUpdatedMaxActorsToReplicate;
	// Custom density spawning parameters.
	bool bInitializedCustomSpawnParameters;
	// Total number of players that will connect. Used to determine number of clusters and spawn points to create.
	int32 TotalPlayers;
	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	// NPCs will be spread out evenly over the created player clusters.
	int32 TotalNPCs;
	int32 NumPlayerClusters;
	int32 PlayersSpawned;
	bool bExitOnDisconnect; // Can speed up NFR testing
	TArray<AActor*> SpawnPoints;
	TSubclassOf<APawn> NPCPawnClass;
	TMap<int32, AActor*> PlayerIdToSpawnPointMap;
	FRandomStream RNG;
	int32 NPCSToSpawn;
	float SecondsTillPlayerCheck;
	void Tick(float DeltaSeconds) override;
	bool ShouldUseCustomSpawning();
	void CheckCmdLineParameters();
	void ParsePassedValues();
	void ClearExistingSpawnPoints();
	void SpawnNPCs(int NumNPCs);
	void SpawnNPC(const FVector& SpawnLocation);
	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	static void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& NumRows, int& NumCols, int& MinRelativeX, int& MinRelativeY);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
};
