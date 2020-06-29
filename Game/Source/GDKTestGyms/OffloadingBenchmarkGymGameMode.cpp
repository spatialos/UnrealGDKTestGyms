// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "OffloadingBenchmarkGymGameMode.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogOffloadingBenchmarkGymGameMode);

AOffloadingBenchmarkGymGameMode::AOffloadingBenchmarkGymGameMode()
{
	ConstructorHelpers::FClassFinder<AActor> NPCBPClassFinder(TEXT("/Game/OffloadingBenchmark/OffloadingBenchmark_NPC_BP"));
	NPCBPClass = NPCBPClassFinder.Class;

	ConstructorHelpers::FClassFinder<AActor>  SimulatedPlayerBPClassFinder(TEXT("/Game/OffloadingBenchmark/OffloadingBenchmark_NPC_BP"));
	SimulatedPlayerBPClass = SimulatedPlayerBPClassFinder.Class;
}

void AOffloadingBenchmarkGymGameMode::BuildExpectedObjectCounts()
{
	Super::BuildExpectedObjectCounts();

	{
		FExpectedActorCount ExpectedActorCount;
		ExpectedActorCount.ActorClass = NPCBPClass;
		ExpectedActorCount.ExpectedCount = TotalNPCs;
		ExpectedActorCount.Variance = 1;
		ExpectedActorCounts.Add(ExpectedActorCount);
	}

	{
		FExpectedActorCount ExpectedActorCount;
		ExpectedActorCount.ActorClass = SimulatedPlayerBPClass;
		ExpectedActorCount.ExpectedCount = RequiredPlayers;
		ExpectedActorCount.Variance = 1;
		ExpectedActorCounts.Add(ExpectedActorCount);
	}
}