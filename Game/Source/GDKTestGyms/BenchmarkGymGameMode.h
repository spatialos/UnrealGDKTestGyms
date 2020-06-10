// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"
#include "BenchmarkGymNPCSpawner.h"

//#include "BenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameMode, Log, All);

typedef TPair<TWeakObjectPtr<AController>, int> ControllerIntegerPair;

/**
 *
 */
CLASS()
class GDKTESTGYMS_API ABenchmarkGymGameMode : public ABenchmarkGymGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameMode();

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
private:
	void GenerateTestScenarioLocations();

	TArray<ControllerIntegerPair> AIControlledPlayers;

	virtual void Tick(float DeltaSeconds) override;
	virtual void ParsePassedValues() override;

	// Custom density spawning parameters.
	bool bInitializedCustomSpawnParameters;
	int32 PlayersSpawned;
	int32 NumPlayerClusters;
	int32 PlayerDensity;
	TMap<int32, FVector> PlayerIdToSpawnPointMap;
	BenchmarkGymSpawnLocations SpawnLocations;

	bool ShouldUseCustomSpawning();
	void CheckCmdLineParameters();
	void ClearExistingSpawnPoints();
	void SetTotalNPCs_Implementation(int32 Value) override;
	void SpawnNPCs(int NumNPCs);
	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	static void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& NumRows, int& NumCols, int& MinRelativeX, int& MinRelativeY);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
};
