// Fill out your copyright notice in the Description page of Project Settings.


#include "BenchmarkGymGameMode.h"
#include "SpatialWorkerFlags.h"
#include "Misc/CommandLine.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGym);

ABenchmarkGymGameMode::ABenchmarkGymGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/Characters/PlayerCharacter_BP"));
	DefaultPawnClass = PawnClass.Class;
	PlayerControllerClass = APlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> NPCBPClass(TEXT("/Game/Characters/SimulatedPlayers/BenchmarkSimulatedPlayer_BP"));
	if (NPCBPClass.Class != NULL) 
	{
		NPCPawnClass = SimulatedPawnClass = NPCBPClass.Class;
	}

	bHasUpdatedMaxActorsToReplicate = false;
	bInitializedCustomSpawnParameters = false;

	TotalPlayers = 1;
	TotalNPCs = 0;
	NumPlayerClusters = 4;
	PlayersSpawned = 0;

	// Seamless Travel is not currently supported in SpatialOS [UNR-897]
	bUseSeamlessTravel = false;

	RNG.Initialize(123456); // Ensure we can do deterministic runs
}

void ABenchmarkGymGameMode::CheckInitCustomSpawning()
{
	if (bInitializedCustomSpawnParameters)
	{
		return;
	}

	if (ShouldUseCustomSpawning())
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Enabling custom density spawning."));
		ParsePassedValues();
		ClearExistingSpawnPoints();

		SpawnPoints.Reset();
		GenerateSpawnPointClusters(NumPlayerClusters);

		if (SpawnPoints.Num() != TotalPlayers) 
		{
			UE_LOG(LogBenchmarkGym, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), SpawnPoints.Num(), TotalPlayers);
		}

		SpawnNPCs(TotalNPCs);
	}
	else
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Custom density spawning disabled."));
	}

	bInitializedCustomSpawnParameters = true;
}

bool ABenchmarkGymGameMode::ShouldUseCustomSpawning()
{
	FString WorkerValue;
	USpatialWorkerFlags::GetWorkerFlag(TEXT("override_spawning"), WorkerValue);
	return (WorkerValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || FParse::Param(FCommandLine::Get(), TEXT("OverrideSpawning")));
}

void ABenchmarkGymGameMode::ParsePassedValues()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("OverrideSpawning")))
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Found OverrideSpawning in command line args, worker flags for custom spawning will be ignored."));
		FParse::Value(FCommandLine::Get(), TEXT("TotalPlayers="), TotalPlayers);

		// Set default value of PlayerDensity equal to TotalPlayers. Will be overwritten if PlayerDensity option is specified.
		PlayerDensity = TotalPlayers;

		FParse::Value(FCommandLine::Get(), TEXT("PlayerDensity="), PlayerDensity);

		FParse::Value(FCommandLine::Get(), TEXT("TotalNPCs="), TotalNPCs);
	}
	else
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString TotalPlayersString, PlayerDensityString, TotalNPCsString;
		if (USpatialWorkerFlags::GetWorkerFlag(TEXT("total_players"), TotalPlayersString))
		{
			TotalPlayers = FCString::Atoi(*TotalPlayersString);
		}

		// Set default value of PlayerDensity equal to TotalPlayers. Will be overwritten if PlayerDensity option is specified.
		PlayerDensity = TotalPlayers;

		if (USpatialWorkerFlags::GetWorkerFlag(TEXT("player_density"), PlayerDensityString))
		{
			PlayerDensity = FCString::Atoi(*PlayerDensityString);
		}

		if (USpatialWorkerFlags::GetWorkerFlag(TEXT("total_npcs"), TotalNPCsString))
		{
			TotalNPCs = FCString::Atoi(*TotalNPCsString);
		}
	}

	NumPlayerClusters = FMath::CeilToInt(TotalPlayers / (float)PlayerDensity);
}

void ABenchmarkGymGameMode::ClearExistingSpawnPoints()
{
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), SpawnPoints);
	for (AActor* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->Destroy();
	}
}

void ABenchmarkGymGameMode::GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& NumRows, int& NumCols, int& MinRelativeX, int& MinRelativeY)
{
	if (NumPoints <= 0)
	{
		UE_LOG(LogBenchmarkGym, Warning, TEXT("Generating grid settings with non-postive number of points (%d)"), NumPoints);
		NumRows = 0;
		NumCols = 0;
		MinRelativeX = 0;
		MinRelativeY = 0;
		return;
	}

	NumRows = FMath::RoundToInt(FMath::Sqrt(NumPoints));
	NumCols = FMath::CeilToInt(NumPoints / (float)NumRows);
	const int GridWidth = (NumCols - 1) * DistBetweenPoints;
	const int GridHeight = (NumRows - 1) * DistBetweenPoints;
	MinRelativeX = FMath::RoundToInt(-GridWidth / 2.0);
	MinRelativeY = FMath::RoundToInt(-GridHeight / 2.0);
}

void ABenchmarkGymGameMode::GenerateSpawnPointClusters(int NumClusters)
{
	const int DistBetweenClusterCenters = 40000; // 400 meters, in Unreal units.
	int NumRows, NumCols, MinRelativeX, MinRelativeY;
	GenerateGridSettings(DistBetweenClusterCenters, NumClusters, NumRows, NumCols, MinRelativeX, MinRelativeY);

	UE_LOG(LogBenchmarkGym, Log, TEXT("Creating player cluster grid of %d rows by %d columns"), NumRows, NumCols);
	for (int i = 0; i < NumClusters; i++)
	{
		const int Row = i % NumRows;
		const int Col = i / NumRows;

		const int ClusterCenterX = MinRelativeX + Col * DistBetweenClusterCenters;
		const int ClusterCenterY = MinRelativeY + Row * DistBetweenClusterCenters;

		GenerateSpawnPoints(ClusterCenterX, ClusterCenterY, PlayerDensity);
	}
}

void ABenchmarkGymGameMode::GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum)
{
	// Spawn in the air above terrain obstacles (Unreal units).
	const int Z = 300;

	const int DistBetweenSpawnPoints = 300; // In Unreal units.
	int NumRows, NumCols, MinRelativeX, MinRelativeY;
	GenerateGridSettings(DistBetweenSpawnPoints, SpawnPointsNum, NumRows, NumCols, MinRelativeX, MinRelativeY);

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogBenchmarkGym, Error, TEXT("Cannot spawn spawnpoints, world is null"));
		return;
	}

	const int WalkRadius = 200; // In Unreal units. More than actual radius, just to be sure.
	const int MaxXDistance = (NumCols - 1) * DistBetweenSpawnPoints + 2 * WalkRadius;
	const int MaxYDistance = (NumRows - 1) * DistBetweenSpawnPoints + 2 * WalkRadius;
	const int MaxDistanceBetweenPlayers = FMath::Sqrt(FMath::Square(MaxXDistance) + FMath::Square(MaxYDistance));
	if (MaxDistanceBetweenPlayers >= PlayerCheckoutRadius)
	{
		UE_LOG(LogBenchmarkGym, Error, TEXT("Maximum distance between players is larger than checkout radius. Cannot ensure players will always see each other."));
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
		UE_LOG(LogBenchmarkGym, Log, TEXT("Creating a new PlayerStart at location %s."), *SpawnLocation.ToString());
		SpawnPoints.Add(World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnInfo));
	}
}

void ABenchmarkGymGameMode::SpawnNPCs(int NumNPCs)
{
	for (int32 i = 0; i < NumNPCs; i++)
	{
		int32 Cluster = i % NumPlayerClusters;
		int32 SpawnPointIndex = Cluster * PlayerDensity;
		AActor* SpawnPoint = SpawnPoints[SpawnPointIndex];
		SpawnNPC(SpawnPoint->GetActorLocation());
	}
}

void ABenchmarkGymGameMode::SpawnNPC(const FVector& SpawnLocation)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogBenchmarkGym, Error, TEXT("Error spawning NPC, World is null"));
		return;
	}

	if (NPCPawnClass == nullptr)
	{
		UE_LOG(LogBenchmarkGym, Error, TEXT("Error spawning NPC, NPCPawnClass is not set."));
		return;
	}

	FVector RandomOffset = RNG.GetUnitVector()*600.0f;
	if (RandomOffset.Z < 0.0f) 
		RandomOffset.Z = -RandomOffset.Z;
	FVector RandomSpawn = SpawnLocation + RandomOffset;
	UE_LOG(LogBenchmarkGym, Log, TEXT("Spawning NPC at %s"), *SpawnLocation.ToString());
	FActorSpawnParameters SpawnInfo{};
	//SpawnInfo.Owner = this;
	//SpawnInfo.Instigator = NULL;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<APawn>(NPCPawnClass->GetDefaultObject()->GetClass(), RandomSpawn, FRotator::ZeroRotator, SpawnInfo);
}

void ABenchmarkGymGameMode::StartPlay()
{
	UE_LOG(LogBenchmarkGym, Log, TEXT("Enabling custom density spawning."));
	ParsePassedValues();
	ClearExistingSpawnPoints();

	SpawnPoints.Reset();
	GenerateSpawnPointClusters(NumPlayerClusters);

	if (SpawnPoints.Num() != TotalPlayers) 
	{
		UE_LOG(LogBenchmarkGym, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), SpawnPoints.Num(), TotalPlayers);
	}

	SpawnNPCs(TotalNPCs);
	return Super::StartPlay();
}
AActor* ABenchmarkGymGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	CheckInitCustomSpawning();

	if (!ShouldUseCustomSpawning() || SpawnPoints.Num() == 0)
	{
		return Super::FindPlayerStart_Implementation(Player, IncomingName);
	}

	// Use custom spawning with density controls
	const int32 PlayerUniqueID = Player->GetUniqueID();
	if (PlayerIdToSpawnPointMap.Contains(PlayerUniqueID))
	{
		return PlayerIdToSpawnPointMap[PlayerUniqueID];
	}

	AActor* ChosenSpawnPoint = SpawnPoints[PlayersSpawned % SpawnPoints.Num()];
	PlayerIdToSpawnPointMap.Add(PlayerUniqueID, ChosenSpawnPoint);

	UE_LOG(LogBenchmarkGym, Log, TEXT("Spawning player %d at %s."), PlayerUniqueID, *ChosenSpawnPoint->GetActorLocation().ToString());

	PlayersSpawned++;

	return ChosenSpawnPoint;
}
