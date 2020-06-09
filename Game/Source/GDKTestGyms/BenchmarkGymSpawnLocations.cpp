// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymSpawnLocations.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymSpawnLocations);

void BenchmarkGymSpawnLocations::Init(int32 Seed, int Num, float Radius);
{
	float Radius = 7500.0f /* Half NCD distance */

	FRandomStream NPCStream;
	NPCStream.Initialize(FCrc::MemCrc32(&Seed, sizeof(Seed)));
	for (int i = 0; i < TotalNPCs; i++)
	{
		FVector PointA = NPCStream.VRand()*Radius;
		FVector PointB = NPCStream.VRand()*Radius;
		PointA.Z = PointB.Z = 0.0f;
		NPCRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
	}
}

FBlackboardValues BenchmarkGymSpawnLocations::GetSpawn(int Index)
{
	if (Index >= RunLocations.Num())
	{
		UE_LOG(LogBenchmarkGymGameMode, Warning, TEXT("Requesting spawn location which was not generated. (%d - %d generated)"), Index, RunLocations.Num());
	}
	return RunLocations
}

void BenchmarkGymSpawnLocations::GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum)
{
	// Spawn in the air above terrain obstacles (Unreal units).
	const int Z = 300;

	const int DistBetweenSpawnPoints = 300; // In Unreal units.
	int NumRows, NumCols, MinRelativeX, MinRelativeY;
	GenerateGridSettings(DistBetweenSpawnPoints, SpawnPointsNum, NumRows, NumCols, MinRelativeX, MinRelativeY);

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogBenchmarkGymSpawnLocations, Error, TEXT("Cannot spawn spawnpoints, world is null"));
		return;
	}

	for (int i = 0; i < SpawnPointsNum; i++)
	{
		const int Row = i % NumRows;
		const int Col = i / NumRows;

		const int X = CenterX + MinRelativeX + Col * DistBetweenSpawnPoints;
		const int Y = CenterY + MinRelativeY + Row * DistBetweenSpawnPoints;

		FActorSpawnParameters SpawnInfo{};
		SpawnInfo.Owner = this;
		SpawnInfo.Instigator = NULL;
		SpawnInfo.bDeferConstruction = false;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector SpawnLocation = FVector(X, Y, Z);
		UE_LOG(LogBenchmarkGymSpawnLocations, Log, TEXT("Creating a new PlayerStart at location %s."), *SpawnLocation.ToString());
		SpawnPoints.Add(SpawnLocation);
	}
}

void BenchmarkGymSpawnLocations::GenerateSpawnPointClusters(int NumClusters)
{
	const int DistBetweenClusterCenters = 40000; // 400 meters, in Unreal units.
	int NumRows, NumCols, MinRelativeX, MinRelativeY;
	GenerateGridSettings(DistBetweenClusterCenters, NumClusters, NumRows, NumCols, MinRelativeX, MinRelativeY);

	UE_LOG(LogBenchmarkGymSpawnLocations, Log, TEXT("Creating player cluster grid of %d rows by %d columns"), NumRows, NumCols);
	for (int i = 0; i < NumClusters; i++)
	{
		const int Row = i % NumRows;
		const int Col = i / NumRows;

		const int ClusterCenterX = MinRelativeX + Col * DistBetweenClusterCenters;
		const int ClusterCenterY = MinRelativeY + Row * DistBetweenClusterCenters;

		GenerateSpawnPoints(ClusterCenterX, ClusterCenterY, PlayerDensity);
	}
}

void BenchmarkGymSpawnLocations::GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY)
{
	if (NumPoints <= 0)
	{
		UE_LOG(LogBenchmarkGymSpawnLocations, Warning, TEXT("Generating grid settings with non-postive number of points (%d)"), NumPoints);
		OutNumRows = 0;
		OutNumCols = 0;
		OutMinRelativeX = 0;
		OutMinRelativeY = 0;
		return;
	}

	OutNumRows = FMath::RoundToInt(FMath::Sqrt(NumPoints));
	OutNumCols = FMath::CeilToInt(NumPoints / static_cast<float>(OutNumRows));
	const int GridWidth = (OutNumCols - 1) * DistBetweenPoints;
	const int GridHeight = (OutNumRows - 1) * DistBetweenPoints;
	OutMinRelativeX = FMath::RoundToInt(-GridWidth / 2.0);
	OutMinRelativeY = FMath::RoundToInt(-GridHeight / 2.0);
}
