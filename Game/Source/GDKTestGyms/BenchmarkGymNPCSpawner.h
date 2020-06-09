// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "BenchmarkGymNPCSpawner.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymNPCSpawner, Log, All);

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API UBenchmarkGymNPCSpawner : public AActor
{
	GENERATED_BODY()
public:
	UBenchmarkGymNPCSpawner();

	void Tick(float DeltaSeconds) override;
private:

	TSubclassOf<APawn> NPCPawnClass;

	int NumSpawned;
	int NumToSpawn;
	
	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);

	UFUNCTION(Reliable, CrossServer)
	void ServerAuthoritiveSpawnNPCs(int NumNPCs);
};
