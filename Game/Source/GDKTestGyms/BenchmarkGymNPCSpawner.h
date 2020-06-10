// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "BlackboardValues.h"
#include "BenchmarkGymSpawnLocations.h"
#include "BenchmarkGymNPCSpawner.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymNPCSpawner, Log, All);

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API ABenchmarkGymNPCSpawner : public AActor
{
	GENERATED_BODY()
public:
	ABenchmarkGymNPCSpawner();

	void Tick(float DeltaSeconds) override;
	
	UFUNCTION(Reliable, CrossServer)
	void ServerAuthoritiveSpawnNPCs(int32 NumNPCs, int32 Clusters, int32 Density);
	void ServerAuthoritiveSpawnNPCs_Implementation(int32 NumNPCs, int32 Clusters, int32 Density)
	{
		NumToSpawn = NumNPCs;
		SpawnLocations.Init(NumNPCs, NumNPCs, Clusters, Density, 7500.0f /* Half NCD */);
		NumClusters = Clusters;
		PlayerDensity = Density;
	}
private:
	BenchmarkGymSpawnLocations SpawnLocations;

	TSubclassOf<APawn> NPCPawnClass;

	int NumSpawned;
	int NumToSpawn;
	int NumClusters;
	int PlayerDensity;

	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);
};
