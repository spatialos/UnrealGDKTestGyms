// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameMode.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BenchmarkGymNPCSpawner.h"
#include "DeterministicBlackboardValues.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "EngineUtils.h"
#include "GDKTestGymsGameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "NFRConstants.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameMode);

namespace
{
	const FString PlayerDensityWorkerFlag = TEXT("player_density");
	const float PercentageSpawnpointsOnWorkerBoundary = 0.25f;
} // anonymous namespace

ABenchmarkGymGameMode::ABenchmarkGymGameMode()
	: bInitializedCustomSpawnParameters(false)
	, NumPlayerClusters(1)
	, PlayersSpawned(0)
	, NPCSToSpawn(0)
{
	PrimaryActorTick.bCanEverTick = true;
	 
	static ConstructorHelpers::FClassFinder<AActor> DropCubeClassFinder(TEXT("/Game/Benchmark/DropCube_BP"));
	DropCubeClass = DropCubeClassFinder.Class;
}

void ABenchmarkGymGameMode::GenerateTestScenarioLocations()
{
	constexpr float RoamRadius = 7500.0f; // Set to half the NetCullDistance
	{
		FRandomStream PlayerStream;
		PlayerStream.Initialize(FCrc::MemCrc32(&ExpectedPlayers, sizeof(ExpectedPlayers))); // Ensure we can do deterministic runs
		for (int i = 0; i < ExpectedPlayers; i++)
		{
			FVector PointA = PlayerStream.VRand()*RoamRadius;
			FVector PointB = PlayerStream.VRand()*RoamRadius;
			PointA.Z = PointB.Z = 0.0f;
			PlayerRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
		}
	}
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

void ABenchmarkGymGameMode::CheckCmdLineParameters()
{
	if (bInitializedCustomSpawnParameters)
	{
		return;
	}

	ParsePassedValues();
	StartCustomNPCSpawning();

	bInitializedCustomSpawnParameters = true;
}

void ABenchmarkGymGameMode::StartCustomNPCSpawning()
{
	ClearExistingSpawnPoints();
	GenerateSpawnPointClusters(NumPlayerClusters);

	if (SpawnPoints.Num() < ExpectedPlayers) // SpawnPoints can be rounded up if ExpectedPlayers % NumClusters != 0
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), SpawnPoints.Num(), ExpectedPlayers);
	}

	GenerateTestScenarioLocations();

	SpawnNPCs(TotalNPCs);
}

void ABenchmarkGymGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		if (NPCSToSpawn > 0)
		{
			int32 NPCIndex = --NPCSToSpawn;
			int32 Cluster = NPCIndex % NumPlayerClusters;
			int32 SpawnPointIndex = Cluster * PlayerDensity;
			const AActor* SpawnPoint = SpawnPoints[SpawnPointIndex];
			SpawnNPC(SpawnPoint->GetActorLocation(), NPCRunPoints[NPCIndex % NPCRunPoints.Num()]);
		}

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
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));

		USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver());
		if (ensure(NetDriver != nullptr))
		{
			const USpatialWorkerFlags* SpatialWorkerFlags = NetDriver->SpatialWorkerFlags;
			if (ensure(SpatialWorkerFlags != nullptr))
			{
				FString PlayerDensityString;
				if(SpatialWorkerFlags->GetWorkerFlag(PlayerDensityWorkerFlag, PlayerDensityString))
				{
					PlayerDensity = FCString::Atoi(*PlayerDensityString);
				}
			}
		}

	}
	NumPlayerClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Density %d, Clusters %d"), PlayerDensity, NumPlayerClusters);
}

void ABenchmarkGymGameMode::OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue)
{
	Super::OnAnyWorkerFlagUpdated(FlagName, FlagValue);
	if (FlagName == PlayerDensityWorkerFlag)
	{
		PlayerDensity = FCString::Atoi(*FlagValue);
	}
}

void ABenchmarkGymGameMode::BuildExpectedActorCounts()
{
	Super::BuildExpectedActorCounts();

	const int32 TotalDropCubes = TotalNPCs + ExpectedPlayers;
	const int32 DropCubeCountVariance = FMath::CeilToInt(TotalDropCubes * 0.1f) + 2;
	AddExpectedActorCount(DropCubeClass, TotalDropCubes, DropCubeCountVariance);
}

void ABenchmarkGymGameMode::ClearExistingSpawnPoints()
{
	for (AActor* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->Destroy();
	}
	SpawnPoints.Reset();
	PlayerIdToSpawnPointMap.Reset();
}

void ABenchmarkGymGameMode::GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY)
{
	if (NumPoints <= 0)
	{
		UE_LOG(LogBenchmarkGymGameMode, Warning, TEXT("Generating grid settings with non-postive number of points (%d)"), NumPoints);
		OutNumRows = 0;
		OutNumCols = 0;
		OutMinRelativeX = 0;
		OutMinRelativeY = 0;
		return;
	}

	OutNumRows = FMath::RoundToInt(FMath::Sqrt(NumPoints));
	OutNumCols = FMath::CeilToInt(NumPoints / static_cast<float>(OutNumRows));
	const int GridWidth = (OutNumCols - 1) * DistBetweenPoints;
	const int GridHeight = (OutNumRows - 1) * DistBetweenPoints;
	OutMinRelativeX = FMath::RoundToInt(-GridWidth / 2.0);
	OutMinRelativeY = FMath::RoundToInt(-GridHeight / 2.0);
}

void ABenchmarkGymGameMode::GenerateSpawnPointClusters(int NumClusters)
{
	const int DistBetweenClusterCenters = 40000; // 400 meters, in Unreal units.

	const int32 SpawnZones = GetNumSpawnZones();

	// We use a fixed size 10km
	const float ZoneWidth = 1000000.0f / SpawnZones;
	const float StartingX = -500000.0f;

	if (SpawnZones > 1)
	{
		// For multiworker configuration we will place a percentage of spawn points
		// on/near the boundary between two zones
		int ClustersOnBoundaries = FMath::CeilToInt(NumClusters * PercentageSpawnpointsOnWorkerBoundary);
		int RemainingClusters = NumClusters - ClustersOnBoundaries;

		TArray<float> Boundaries;
		for (int i = 1; i < SpawnZones; ++i)
		{
			Boundaries.Emplace(StartingX + ZoneWidth * i);
		}

		int NumPerBoundary = FMath::CeilToInt(ClustersOnBoundaries / static_cast<float>(Boundaries.Num()));
		int StartingY = FMath::RoundToInt(-((NumPerBoundary - 1) * DistBetweenClusterCenters) / 2.0f);
		int BoundaryIndex = 0;
		while(ClustersOnBoundaries > 0)
		{
			int ClusterCenterX = FMath::RoundToInt(Boundaries[BoundaryIndex]);
			for (int y = 0; y < NumPerBoundary && ClustersOnBoundaries > 0; ++y)
			{
				int ClusterCenterY = StartingY + y * DistBetweenClusterCenters;
				//Because GridBaseLBStrategy swaps X and Y, we will swap them here so that we're aligned
				GenerateSpawnPoints(ClusterCenterY, ClusterCenterX, PlayerDensity);
				UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Creating player cluster near boundary location: %d , %d"), ClusterCenterY, ClusterCenterX);
				--ClustersOnBoundaries;
			}
			++BoundaryIndex;
		}

		NumClusters = RemainingClusters;
	}

	int ClustersPerWorker = FMath::CeilToInt(NumClusters / static_cast<float>(SpawnZones));
	for (int w = 0; w < SpawnZones; ++w)
	{
		int ClusterCount = FMath::Min(ClustersPerWorker, NumClusters);
		NumClusters -= ClusterCount;
		int NumRows, NumCols, MinRelativeX, MinRelativeY;
		GenerateGridSettings(DistBetweenClusterCenters, ClusterCount, NumRows, NumCols, MinRelativeX, MinRelativeY);

		//Adjust the lefthand side of the grid to so that the grid is centered in the zone
		MinRelativeX = FMath::RoundToInt(MinRelativeX + StartingX + (w * ZoneWidth) + (ZoneWidth / 2.0f));

		UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Creating player cluster grid of %d rows by %d columns from location: %d , %d"), NumRows, NumCols, MinRelativeX, MinRelativeY);
		for (int i = 0; i < ClusterCount; i++)
		{
			const int Row = i % NumRows;
			const int Col = i / NumRows;

			const int ClusterCenterX = MinRelativeX + Col * DistBetweenClusterCenters;
			const int ClusterCenterY = MinRelativeY + Row * DistBetweenClusterCenters;
			//Because GridBaseLBStrategy swaps X and Y, we will swap them here so that we're aligned
			GenerateSpawnPoints(ClusterCenterY, ClusterCenterX, PlayerDensity);
		}
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
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Cannot spawn spawnpoints, world is null"));
		return;
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
		UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Creating a new PlayerStart at location %s."), *SpawnLocation.ToString());
		SpawnPoints.Add(World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnInfo));
	}
}

void ABenchmarkGymGameMode::SpawnNPCs(int NumNPCs)
{
	NPCSToSpawn = NumNPCs;
}

void ABenchmarkGymGameMode::SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Error spawning NPC, World is null"));
		return;
	}

	if (NPCSpawner == nullptr)
	{
		ABenchmarkGymNPCSpawner* Spawner = nullptr;
		for (TActorIterator<ABenchmarkGymNPCSpawner> It(GetWorld()); It; ++It)
		{
			Spawner = *It;
			break;
		}
		NPCSpawner = Spawner;
	}
	if (NPCSpawner != nullptr)
	{
		NPCSpawner->CrossServerSpawn(NPCClass, SpawnLocation, BlackboardValues);
	}
	else
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Failed to find NPCSpawner."));
	}
}

AActor* ABenchmarkGymGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	CheckCmdLineParameters();

	if (SpawnPoints.Num() == 0)
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
