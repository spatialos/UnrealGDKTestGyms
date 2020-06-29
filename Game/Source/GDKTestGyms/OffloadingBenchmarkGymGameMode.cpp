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

double AOffloadingBenchmarkGymGameMode::GetOffloadingBenchmarkNPCs() const
{
	return GetActorClassCount(NPCBPClass);
}

double AOffloadingBenchmarkGymGameMode::GetOffloadingBenchmarkSimulatedPlayers() const
{
	return GetActorClassCount(SimulatedPlayerBPClass);
}

void AOffloadingBenchmarkGymGameMode::BuildExpectedObjectCounts()
{
	Super::BuildExpectedObjectCounts();

	{
		FExpectedObjectCount ExpectedObjectCount;
		ExpectedObjectCount.ObjectClass = NPCBPClass;
		ExpectedObjectCount.ExpectedCount = TotalNPCs;
		ExpectedObjectCount.Variance = 1;
		ExpectedObjectCount.ActorCountDelegate.BindUObject(this, &AOffloadingBenchmarkGymGameMode::GetOffloadingBenchmarkNPCs);
		ExpectedObjectCounts.Add(ExpectedObjectCount);
	}

	{
		FExpectedObjectCount ExpectedObjectCount;
		ExpectedObjectCount.ObjectClass = SimulatedPlayerBPClass;
		ExpectedObjectCount.ExpectedCount = RequiredPlayers;
		ExpectedObjectCount.Variance = 1;
		ExpectedObjectCount.ActorCountDelegate.BindUObject(this, &AOffloadingBenchmarkGymGameMode::GetOffloadingBenchmarkNPCs);
		ExpectedObjectCounts.Add(ExpectedObjectCount);
	}
}