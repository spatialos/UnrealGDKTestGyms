// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameMode.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DeterministicBlackboardValues.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "GDKTestGymsGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Utils/SpatialMetrics.h"
#include "Utils/SpatialStatics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameMode);

ABenchmarkGymGameMode::ABenchmarkGymGameMode()
	: bInitializedCustomSpawnParameters(false)
	, NumPlayerClusters(1)
	, PlayersSpawned(0)
	, NPCSToSpawn(0)
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/Characters/PlayerCharacter_BP"));
	DefaultPawnClass = PawnClass.Class;
	PlayerControllerClass = APlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> SimulatedBPPawnClass(TEXT("/Game/Characters/SimulatedPlayers/SimulatedPlayerCharacter_BP"));
	if (SimulatedBPPawnClass.Class != NULL)
	{
		SimulatedPawnClass = SimulatedBPPawnClass.Class;
	}
}

void ABenchmarkGymGameMode::GenerateTestScenarioLocations()
{
	constexpr float RoamRadius = 7500.0f; // Set to half the NetCullDistance
	
	{
		FRandomStream NPCStream;
		NPCStream.Initialize(FCrc::MemCrc32(&TotalNPCs, sizeof(TotalNPCs)));
		for (int i = 0; i < TotalNPCs; i++)
		{
			FVector PointA = NPCStream.VRand()*RoamRadius;
			FVector PointB = NPCStream.VRand()*RoamRadius;
			PointA.Z = PointB.Z = 0.0f;
			NPCRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
		}
	}
}

void ABenchmarkGymGameMode::SetTotalNPCs_Implementation(int32 Value)
{
	CheckCmdLineParameters();
	Super::SetTotalNPCs_Implementation(Value);
	UE_LOG(LogBenchmarkGymGameMode, Warning, TEXT("Spawning %d NPCs"), TotalNPCs);
	SpawnNPCs(TotalNPCs);
}

void ABenchmarkGymGameMode::CheckCmdLineParameters()
{
	if (bInitializedCustomSpawnParameters)
	{
		return;
	}
	bInitializedCustomSpawnParameters = true;

	ParsePassedValues();

	if (ShouldUseCustomSpawning())
	{
		UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Enabling custom density spawning."));
		ClearExistingSpawnPoints();

		SpawnPoints.Reset();
		GenerateSpawnPointClusters(NumPlayerClusters);

		if (SpawnPoints.Num() != ExpectedPlayers)
		{
			UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), SpawnPoints.Num(), ExpectedPlayers);
		}

		GenerateTestScenarioLocations();
	}
	else
	{
		UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Custom density spawning disabled."));
	}
}

void ABenchmarkGymGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int i = AIControlledPlayers.Num() - 1; i >= 0; i--)
	{
		AController* Controller = AIControlledPlayers[i].Key.Get();
		int InfoIndex = AIControlledPlayers[i].Value;

		checkf(Controller, TEXT("Simplayer controller has been deleted."));
		ACharacter* Character = Controller->GetCharacter();
		checkf(Character, TEXT("Simplayer character does not exist."));
		UDeterministicBlackboardValues* Blackboard = Cast<UDeterministicBlackboardValues>(Character->FindComponentByClass(UDeterministicBlackboardValues::StaticClass()));
		checkf(Blackboard, TEXT("Simplayer does not have a UDeterministicBlackboardValues component."));

		const FBlackboardValues& Points = PlayerRunPoints[InfoIndex % PlayerRunPoints.Num()];
		Blackboard->ClientSetBlackboardAILocations(Points);
	}
	AIControlledPlayers.Empty();
}

bool ABenchmarkGymGameMode::ShouldUseCustomSpawning()
{
	FString WorkerValue;
	if (USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver()))
	{
		NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("override_spawning"), WorkerValue);
	}
	return (WorkerValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || FParse::Param(FCommandLine::Get(), TEXT("OverrideSpawning")));
}

void ABenchmarkGymGameMode::ParsePassedValues()
{
	Super::ParsePassedValues();

	PlayerDensity = ExpectedPlayers;

	const FString& CommandLine = FCommandLine::Get();
	if (FParse::Param(*CommandLine, *ReadFromCommandLineKey))
	{
		FParse::Value(*CommandLine, TEXT("PlayerDensity="), PlayerDensity);
	}
	else if(GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver());
		check(NetDriver);

		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString PlayerDensityString;

		const USpatialWorkerFlags* SpatialWorkerFlags = NetDriver != nullptr ? NetDriver->SpatialWorkerFlags : nullptr;
		check(SpatialWorkerFlags != nullptr);

		if (SpatialWorkerFlags->GetWorkerFlag(TEXT("player_density"), PlayerDensityString))
		{
			PlayerDensity = FCString::Atoi(*PlayerDensityString);
		}
	}
	NumPlayerClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Density %d, Clusters %d"), PlayerDensity, NumPlayerClusters);
}

void ABenchmarkGymGameMode::ClearExistingSpawnPoints()
{
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), SpawnPoints);
	for (AActor* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->Destroy();
	}
}

void ABenchmarkGymGameMode::SpawnNPCs(int NumNPCs)
{
	NPCSToSpawn = NumNPCs;
}

AActor* ABenchmarkGymGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	CheckCmdLineParameters();

	if (SpawnPoints.Num() == 0 || !ShouldUseCustomSpawning())
	{
		return Super::FindPlayerStart_Implementation(Player, IncomingName);
	}

	if (Player == nullptr) // Work around for load balancing passing nullptr Player
	{
		return SpawnPoints[PlayersSpawned % SpawnPoints.Num()];
	}

	// Use custom spawning with density controls
	const int32 PlayerUniqueID = Player->GetUniqueID();
	AActor** SpawnPoint = PlayerIdToSpawnPointMap.Find(PlayerUniqueID);
	if (SpawnPoint != nullptr)
	{
		return *SpawnPoint;
	}

	AActor* ChosenSpawnPoint = SpawnPoints[PlayersSpawned % SpawnPoints.Num()];
	PlayerIdToSpawnPointMap.Add(PlayerUniqueID, ChosenSpawnPoint);

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Spawning player %d at %s."), PlayerUniqueID, *ChosenSpawnPoint->GetActorLocation().ToString());
	
	if (Player->GetIsSimulated())
	{
		AIControlledPlayers.Emplace(ControllerIntegerPair{ Player, PlayersSpawned });
	}

	PlayersSpawned++;

	return ChosenSpawnPoint;
}
