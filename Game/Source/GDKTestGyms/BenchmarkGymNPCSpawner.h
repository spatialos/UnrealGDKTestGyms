// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"

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
	UFUNCTION(Reliable, CrossServer)
	void CrossServerSpawn(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);

	ABenchmarkGymNPCSpawner();
private:
	UPROPERTY()
	TSubclassOf<APawn> NPCPawnClass;
};
