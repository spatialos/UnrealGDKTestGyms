// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymNPCSpawner.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DeterministicBlackboardValues.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymNPCSpawner);

ABenchmarkGymNPCSpawner::ABenchmarkGymNPCSpawner()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ABenchmarkGymNPCSpawner::CrossServerSpawn_Implementation(TSubclassOf<APawn> NPCClass, const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogBenchmarkGymNPCSpawner, Error, TEXT("Error spawning NPC, World is null"));
		return;
	}

	if (NPCClass == nullptr)
	{
		UE_LOG(LogBenchmarkGymNPCSpawner, Error, TEXT("Error spawning NPC, NPCClass is not set."));
		return;
	}

	const float RandomSpawnOffset = 600.0f;
	FVector RandomOffset = FMath::VRand()*RandomSpawnOffset;
	if (RandomOffset.Z < 0.0f)
	{
		RandomOffset.Z = -RandomOffset.Z;
	}

	FVector FixedSpawnLocation = SpawnLocation + RandomOffset;
	UE_LOG(LogBenchmarkGymNPCSpawner, Log, TEXT("Spawning NPC at %s"), *SpawnLocation.ToString());
	FActorSpawnParameters SpawnInfo{};
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Pawn = World->SpawnActor<APawn>(NPCClass, FixedSpawnLocation, FRotator::ZeroRotator, SpawnInfo);
	checkf(Pawn, TEXT("Pawn failed to spawn at %s"), *FixedSpawnLocation.ToString());

	UDeterministicBlackboardValues* Comp = Cast<UDeterministicBlackboardValues>(Pawn->FindComponentByClass(UDeterministicBlackboardValues::StaticClass()));
	checkf(Comp, TEXT("Pawn must have a UDeterministicBlackboardValues component."));
	Comp->ClientSetBlackboardAILocations(BlackboardValues);
}
