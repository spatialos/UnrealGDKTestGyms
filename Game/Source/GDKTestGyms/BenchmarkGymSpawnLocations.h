// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "BlackboardValues.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymSpawnLocations, Log, All);

class BenchmarkGymSpawnLocations 
{
public:
	BenchmarkGymSpawnLocations();

	void Init(int32 Seed, int32 NumSpawns, int32 Clusters, int32 Density, float Radius);
	FBlackboardValues GetRunBetweenPoints(int32 Index);
	FVector GetSpawnPoint(int Index);

	bool IsInitialized() { return bIsInitialized; }
private:
	TArray<FBlackboardValues> RunLocations;	
	TArray<FVector> SpawnPoints;
	bool bIsInitialized{ false };

	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	int32 NumPlayerClusters;

	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY);
};
