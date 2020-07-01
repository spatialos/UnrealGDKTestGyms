// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "OffloadingBenchmarkGymGameMode.h"

#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogOffloadingBenchmarkGymGameMode);

AOffloadingBenchmarkGymGameMode::AOffloadingBenchmarkGymGameMode()
{
	ConstructorHelpers::FClassFinder<AActor> NPCBPClassFinder(TEXT("/Game/OffloadingBenchmark/OffloadingBenchmark_NPC_BP"));
	NPCBPClass = NPCBPClassFinder.Class;

	ConstructorHelpers::FClassFinder<AActor>  SimulatedPlayerBPClassFinder(TEXT("/Game/OffloadingBenchmark/OffloadingBenchmark_NPC_BP"));
	SimulatedPlayerBPClass = SimulatedPlayerBPClassFinder.Class;
}

void AOffloadingBenchmarkGymGameMode::BuildExpectedActorCounts()
{
	Super::BuildExpectedActorCounts();

	AddExpectedActorCount(NPCBPClass, TotalNPCs, 1);
	AddExpectedActorCount(SimulatedPlayerBPClass, RequiredPlayers, 1);
}