// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymSpawnLocations, Log, All);

class BenchmarkGymSpawnLocations 
{
public:
	BenchmarkGymSpawnLocations();

	void Init(int32 Seed, int Num, int Density);
	FBlackboardValues GetRunBetweenPoints(int Index);
	FVector3d GetSpawnPoint(int Index);
private:
	TArray<FBlackboardValues> RunLocations;	
	TArray<FVector3d> SpawnPoints;

	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY);
};
