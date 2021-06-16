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
#include "Utils/SpatialStatics.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGymGameMode);

namespace
{
	const FString PlayerDensityWorkerFlag = TEXT("player_density");
	const FString BenchmarkPlayerDensityCommandLineKey = TEXT("PlayerDensity=");

	// Returns an array of co-ordinates. Each co-ordinate represents the center of a cell in a generated grid.
	// The grid created will abide by these rules if the function returns true:
	//		- Fit inside the dimensions GridMaxWidth, GridMaxHeight.
	//		- Have an odd number of rows and columns.
	//		- Have at minimum MinCells cells.
	//		- All center positions will be CellSize apart.

	bool GenerateMinimalGridInArea(const float GridMaxWidth, const float GridMaxHeight, const int32 MinCells, const float CellSize, const FVector& WorldPositon, TArray<FVector>& OutCellCenterPoints)
	{
		if (CellSize == 0)
		{
			UE_LOG(LogBenchmarkGymGameMode, Warning, TEXT("Trying to generate grid settings using invalid CellSize"));
			return false;
		}

		const int32 MaxRows = FMath::FloorToInt(GridMaxHeight / CellSize);
		const int32 MaxCols = FMath::FloorToInt(GridMaxWidth / CellSize);

		// Ensure we have odd number of rows and cols.
		int32 Rows = MaxRows % 2 == 0 ? MaxRows - 1 : MaxRows;
		int32 Cols = MaxCols % 2 == 0 ? MaxCols - 1 : MaxCols;

		if (Rows * Cols < MinCells)
		{
			UE_LOG(LogBenchmarkGymGameMode, Warning, TEXT("Area not big enough for MinCells"));
			return false;
		}

		const float MaxRowMaxColRatio = MaxRows / MaxCols;
		auto EvaluateRowsAndColumns = [MinCells, MaxRowMaxColRatio](const int32 Rows, const int32 Cols, bool& bOutSatisfiesMinCells, float& OutRatioDiff)
		{
			bOutSatisfiesMinCells = Rows > 0 && Cols > 0 && Rows * Cols >= MinCells;
			if (bOutSatisfiesMinCells)
			{
				const float RowColRatio = Rows / static_cast<float>(Cols);
				OutRatioDiff = FMath::Abs(RowColRatio - MaxRowMaxColRatio);
			}
		};

		while (true)
		{
			const int32 NextRows = Rows - 2;
			const int32 NextCols = Cols - 2;

			bool bNextRowsSatisfiesMinCells;
			float NextRowsRowColRatioDiff = 0.0f;
			EvaluateRowsAndColumns(NextRows, Cols, bNextRowsSatisfiesMinCells, NextRowsRowColRatioDiff);

			bool bNextColsSatisfiesMinCells;
			float NextColsRowColRatioDiff = 0.0f;
			EvaluateRowsAndColumns(Rows, NextCols, bNextColsSatisfiesMinCells, NextColsRowColRatioDiff);

			if (bNextRowsSatisfiesMinCells &&
				(NextRowsRowColRatioDiff < NextColsRowColRatioDiff || !bNextColsSatisfiesMinCells))
			{
				Rows = NextRows;
			}
			else if (bNextColsSatisfiesMinCells &&
				(NextRowsRowColRatioDiff >= NextColsRowColRatioDiff || !bNextRowsSatisfiesMinCells))
			{
				Cols = NextCols;
			}
			else
			{
				break;
			}
		}

		const float GridWidth = Cols * CellSize;
		const float GridHeight = Rows * CellSize;

		const float StartX = WorldPositon.X - GridWidth / 2.0f + CellSize / 2.0f;
		const float StartY = WorldPositon.Y - GridHeight / 2.0f + CellSize / 2.0f;

		OutCellCenterPoints.Empty();
		for (int32 Row = 0; Row < Rows; ++Row)
		{
			for (int32 Col = 0; Col < Cols; ++Col)
			{
				const float PointX = Col * CellSize + StartX;
				const float PointY = Row * CellSize + StartY;
				OutCellCenterPoints.Add({ PointX, PointY, 0.0f });
			}
		}

		return true;
	}
} // anonymous namespace

// --- USpawnCluster ---

USpawnCluster::~USpawnCluster()
{
	for (AActor* SpawnPointActor : SpawnPointActors)
	{
		SpawnPointActor->Destroy();
	}
}

void USpawnCluster::GenerateSpawnPoints()
{
	// Spawn points 3m apart to avoid spawn collision issues.
	const float DistBetweenSpawnPoints = 300.0f;
	const bool bSuccefullyCreatedReducedGrid = GenerateMinimalGridInArea(Width, Height, MaxSpawnPoints, DistBetweenSpawnPoints, WorldPosition, SpawnPoints);
	if (!bSuccefullyCreatedReducedGrid)
	{
		return;
	}

	// Sort the spawn points by "distance from centre of cluster" ascending.
	// This ensures we iterate from closest to furthest spawn point.
	SpawnPoints.Sort([this](const FVector& Point1, const FVector& Point2)
	{
		const float Distance1 = FVector::Distance(Point1, WorldPosition);
		const float Distance2 = FVector::Distance(Point2, WorldPosition);
		return Distance2 > Distance1;
	});

	//Remove any excess spawn points
	SpawnPoints.RemoveAt(MaxSpawnPoints, SpawnPoints.Num() - MaxSpawnPoints);
}

const TArray<AActor*>& USpawnCluster::CreateSpawnPointActors()
{
	UWorld* World = GetWorld();

	for (int i = 0; i < SpawnPoints.Num(); ++i)
	{
		const FVector& SpawnPoint = SpawnPoints[i];

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = World->GetAuthGameMode();
		SpawnInfo.bDeferConstruction = false;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Swapping X and Y as GridBaseLBStrategy has the two reversed.
		// Spawn actor is placed 3m off the ground to avoid spawning collisions.
		FVector ModifiedSpawnPoint = SpawnPoint;
		ModifiedSpawnPoint.X = SpawnPoint.Y;
		ModifiedSpawnPoint.Y = SpawnPoint.X;
		ModifiedSpawnPoint.Z = 300.0f;

		UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Creating a new PlayerStart at location %s."), *ModifiedSpawnPoint.ToString());
		SpawnPointActors.Add(World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ModifiedSpawnPoint, FRotator::ZeroRotator, SpawnInfo));
	}

	return SpawnPointActors;
}

// --- USpawnArea ---

void USpawnArea::GenerateSpawnClusters()
{
	TArray<FVector> ClusterPoints;
	const bool bSuccefullyCreatedReducedGrid = GenerateMinimalGridInArea(Width, Height, MaxClusters, MinDistanceBetweenClusters, WorldPosition, ClusterPoints);
	if (!bSuccefullyCreatedReducedGrid)
	{
		return;
	}

	// Sort the clusters by "distance from centre of area" ascending.
	// This ensures we iterate from closest to furthest cluster.
	ClusterPoints.Sort([this](const FVector& Point1, const FVector& Point2)
	{
		const float Distance1 = FVector::Distance(Point1, WorldPosition);
		const float Distance2 = FVector::Distance(Point2, WorldPosition);
		return Distance2 > Distance1;
	});

	//Remove any excess areas
	ClusterPoints.RemoveAt(MaxClusters, ClusterPoints.Num() - MaxClusters);

	for (int32 i = 0; i < ClusterPoints.Num() && i < MaxClusters; ++i)
	{
		const FVector& ClusterPoint = ClusterPoints[i];
		USpawnCluster* NewSpawnCluster = NewObject<USpawnCluster>(this);
		if (NewSpawnCluster != nullptr)
		{
			NewSpawnCluster->WorldPosition = ClusterPoint;
			NewSpawnCluster->Width = MinDistanceBetweenClusters;
			NewSpawnCluster->Height = MinDistanceBetweenClusters;
			NewSpawnCluster->MaxSpawnPoints = MaxSpawnPointsPerCluster;

			NewSpawnCluster->GenerateSpawnPoints();
			SpawnClusters.Add(NewSpawnCluster);
		}
	}
}

TArray<AActor*> USpawnArea::CreateSpawnPointActors()
{
	if (SpawnClusters.Num() == 0)
	{
		return {};
	}

	TArray<AActor*> OutSpawnPointActors;
	for (USpawnCluster* Cluster : SpawnClusters)
	{
		OutSpawnPointActors += Cluster->CreateSpawnPointActors();
	}
	return OutSpawnPointActors;
}

// --- USpawnManager ---

void USpawnManager::GenerateSpawnPoints(const int32 ZoneRows, const int32 ZoneCols, const int32 ZoneWidth, const int32 ZoneHeight,
	const int32 ZoneClusters, const int32 BoundaryClusters,
	const int32 MaxSpawnPointsPerCluster, const int32 MinDistanceBetweenClusters)
{
	GenerateSpawnAreas(ZoneRows, ZoneCols, ZoneWidth, ZoneHeight, ZoneClusters, BoundaryClusters, MaxSpawnPointsPerCluster, MinDistanceBetweenClusters);
	CreateSpawnPointActors(ZoneClusters, BoundaryClusters, MaxSpawnPointsPerCluster);
}

AActor* USpawnManager::GetSpawnPointActorByIndex(const int32 Index) const
{
	const int32 NumSpawnPoints = GetNumSpawnPoints();
	if (NumSpawnPoints == 0)
	{
		return nullptr;
	}
	return SpawnPointActors[Index % NumSpawnPoints];
}

int32 USpawnManager::GetNumSpawnPoints() const
{
	return SpawnPointActors.Num();
}

void USpawnManager::ClearSpawnPoints()
{
	SpawnAreas.Empty();
	SpawnPointActors.Empty();
}

void USpawnManager::GenerateSpawnAreas(const int32 ZoneRows, const int32 ZoneCols, const int32 ZoneWidth, const int32 ZoneHeight,
	const int32 ZoneClusters, const int32 BoundaryClusters,
	const int32 MaxSpawnPointsPerCluster, const int32 MinDistanceBetweenClusters)
{
	const float StartX = ZoneWidth / 2.0f - ZoneCols * ZoneWidth / 2.0f;
	const float StartY = ZoneHeight / 2.0f - ZoneRows * ZoneHeight / 2.0f;
	const int32 SpawnAreaRows = ZoneRows * 2 - 1;
	const int32 SpawnAreaCols = ZoneCols * 2 - 1;
	const int32 SpawnAreaWidth = ZoneWidth - MinDistanceBetweenClusters;
	const int32 SpawnAreaHeight = ZoneHeight - MinDistanceBetweenClusters;

	const int32 NumZones = ZoneRows * ZoneCols;
	const int32 NumBoundaries = ZoneRows * (ZoneCols - 1) + ZoneCols * (ZoneRows - 1);

	const int32 MaxClustersPerZone = FMath::CeilToInt(ZoneClusters / static_cast<float>(NumZones));
	const int32 MaxClustersPerBoundary = NumBoundaries != 0 ? FMath::CeilToInt(BoundaryClusters / static_cast<float>(NumBoundaries)) : 0;

	for (int32 Row = 0; Row < SpawnAreaRows; ++Row)
	{
		for (int32 Col = 0; Col < SpawnAreaCols; ++Col)
		{
			bool bZoneRow = Row % 2 == 0;
			bool bZoneCol = Col % 2 == 0;
			if (!bZoneRow && !bZoneCol)
			{
				// Skip boundary intersections
				continue;
			}

			bool bIsZone = bZoneRow && bZoneCol;
			const int32 MaxClusters = bIsZone ? MaxClustersPerZone : MaxClustersPerBoundary;
			if (MaxClusters == 0)
			{
				// Skip areas that need 0 clusters
				continue;
			}

			USpawnArea* NewSpawnArea = NewObject<USpawnArea>(this);
			if (NewSpawnArea != nullptr)
			{
				NewSpawnArea->Type = bIsZone ? USpawnArea::Type::Zone : USpawnArea::Type::Boundary;

				const float X = StartX + Col * ZoneWidth / 2.0f;
				const float Y = StartY + Row * ZoneHeight / 2.0f;
				NewSpawnArea->WorldPosition = { X, Y, 0.0f };

				NewSpawnArea->Width = bIsZone ? SpawnAreaWidth : bZoneRow ? MinDistanceBetweenClusters : SpawnAreaWidth;
				NewSpawnArea->Height = bIsZone ? SpawnAreaHeight : bZoneCol ? MinDistanceBetweenClusters : SpawnAreaHeight;
				NewSpawnArea->MaxClusters = MaxClusters;
				NewSpawnArea->MaxSpawnPointsPerCluster = MaxSpawnPointsPerCluster;
				NewSpawnArea->MinDistanceBetweenClusters = MinDistanceBetweenClusters;

				NewSpawnArea->GenerateSpawnClusters();
				SpawnAreas.Add(NewSpawnArea);
			}
		}
	}
}

void USpawnManager::CreateSpawnPointActors(int32 ZoneClusters, int32 BoundaryClusters, const int32 MaxSpawnPointsPerCluster)
{
	for (USpawnArea* SpawnArea : SpawnAreas)
	{
		if (SpawnArea->Type == USpawnArea::Type::Boundary && BoundaryClusters > 0)
		{
			SpawnPointActors += SpawnArea->CreateSpawnPointActors();
			BoundaryClusters--;
		}
		else if (SpawnArea->Type == USpawnArea::Type::Zone && ZoneClusters > 0)
		{
			SpawnPointActors += SpawnArea->CreateSpawnPointActors();
			ZoneClusters--;
		}
	}
}

// --- ABenchmarkGymGameMode ---

ABenchmarkGymGameMode::ABenchmarkGymGameMode()
	: DistBetweenClusterCenters(40000) // 400 meters, in Unreal units.
	, PercentageSpawnPointsOnWorkerBoundaries(0.05f)
	, bHasCreatedSpawnPoints(false)
	, PlayerDensity(-1) // PlayerDensity is invalid until set via command line arg or worker flag.
	, PlayersSpawned(0)
	, NPCSToSpawn(0)
{
	PrimaryActorTick.bCanEverTick = true;

	FString DropCubeAssetPath = TEXT("/Game/Benchmark/DropCube_BP");
	static ConstructorHelpers::FClassFinder<AActor> DropCubeClassFinder(*DropCubeAssetPath);
	if (!DropCubeClassFinder.Succeeded())
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Provided DropCube asset path unsuccesfully loaded. Path: %s"), *DropCubeAssetPath);
		return;
	}
	DropCubeClass = DropCubeClassFinder.Class;
}

void ABenchmarkGymGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnManager = NewObject<USpawnManager>(this);
	TryStartCustomNPCSpawning();
}

void ABenchmarkGymGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TryStartCustomNPCSpawning();

	if (HasAuthority())
	{
		if (NPCSToSpawn > 0)
		{
			const int32 NPCIndex = TotalNPCs - NPCSToSpawn;
			const AActor* SpawnPoint = SpawnManager->GetSpawnPointActorByIndex(NPCIndex);
			if (SpawnPoint != nullptr)
			{
				FVector SpawnLocation = SpawnPoint->GetActorLocation();
				SpawnNPC(SpawnLocation, NPCRunPoints[NPCIndex % NPCRunPoints.Num()]);
				NPCSToSpawn--;
			}
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

void ABenchmarkGymGameMode::BindWorkerFlagDelegates(USpatialWorkerFlags* SpatialWorkerFlags)
{
	Super::BindWorkerFlagDelegates(SpatialWorkerFlags);
	{
		FOnWorkerFlagUpdatedBP WorkerFlagDelegate;
		WorkerFlagDelegate.BindDynamic(this, &ABenchmarkGymGameMode::OnPlayerDensityFlagUpdate);
		SpatialWorkerFlags->RegisterFlagUpdatedCallback(PlayerDensityWorkerFlag, WorkerFlagDelegate);
	}
}

void ABenchmarkGymGameMode::ReadCommandLineArgs(const FString& CommandLine)
{
	Super::ReadCommandLineArgs(CommandLine);
	FParse::Value(*CommandLine, *BenchmarkPlayerDensityCommandLineKey, PlayerDensity);

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("PlayersDensity %d"), PlayerDensity);
}

void ABenchmarkGymGameMode::ReadWorkerFlagValues(USpatialWorkerFlags* SpatialWorkerFlags)
{
	Super::ReadWorkerFlagValues(SpatialWorkerFlags);

	FString PlayerDensityString;
	if (SpatialWorkerFlags->GetWorkerFlag(PlayerDensityWorkerFlag, PlayerDensityString))
	{
		PlayerDensity = FCString::Atoi(*PlayerDensityString);
	}

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("PlayersDensity %d"), PlayerDensity);
}

void ABenchmarkGymGameMode::GenerateTestScenarioLocations()
{
	const APawn* Pawn = GetDefault<APawn>(SimulatedPawnClass);
	const float RoamRadius = FMath::Sqrt(Pawn->NetCullDistanceSquared) / 2.0f;
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

void ABenchmarkGymGameMode::TryStartCustomNPCSpawning()
{
	if (ExpectedPlayers == -1 || PlayerDensity == -1 || bHasCreatedSpawnPoints)
	{
		return;
	}

	StartCustomNPCSpawning();
}

void ABenchmarkGymGameMode::StartCustomNPCSpawning()
{
	ClearExistingSpawnPoints();
	GenerateSpawnPoints();

	const int32 NumSpawnPoints = SpawnManager->GetNumSpawnPoints();
	if (NumSpawnPoints < ExpectedPlayers) // SpawnPoints can be rounded up if ExpectedPlayers % NumClusters != 0
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), NumSpawnPoints, ExpectedPlayers);
	}

	GenerateTestScenarioLocations();

	SpawnNPCs(TotalNPCs);

	bHasCreatedSpawnPoints = true;
}

void ABenchmarkGymGameMode::BuildExpectedActorCounts()
{
	Super::BuildExpectedActorCounts();

	const int32 TotalDropCubes = TotalNPCs + ExpectedPlayers;
	const int32 DropCubeCountVariance = FMath::CeilToInt(TotalDropCubes * 0.1f) + 2;
	AddExpectedActorCount(DropCubeClass, TotalDropCubes - DropCubeCountVariance, TotalDropCubes + DropCubeCountVariance);
}

void ABenchmarkGymGameMode::ClearExistingSpawnPoints()
{
	SpawnManager->ClearSpawnPoints();
	PlayerIdToSpawnPointMap.Reset();
}

void ABenchmarkGymGameMode::GenerateSpawnPoints()
{
	const int32 NumClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));
	const int32 BoundaryClusters = FMath::CeilToInt(NumClusters * PercentageSpawnPointsOnWorkerBoundaries);
	const int32 ZoneClusters = NumClusters - BoundaryClusters;

	const int32 Rows = GetZoningRows();
	const int32 Cols = GetZoningCols();
	const float Width = GetZoneWidth();
	const float Height = GetZoneHeight();

	SpawnManager->GenerateSpawnPoints(Rows, Cols, Width, Height, ZoneClusters, BoundaryClusters, PlayerDensity, DistBetweenClusterCenters);
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
	if (SpawnManager->GetNumSpawnPoints() == 0)
	{
		return Super::FindPlayerStart_Implementation(Player, IncomingName);
	}

	if (Player == nullptr) // Work around for load balancing passing nullptr Player
	{
		return SpawnManager->GetSpawnPointActorByIndex(PlayersSpawned);
	}

	// Use custom spawning with density controls
	const int32 PlayerUniqueID = Player->GetUniqueID();
	AActor** SpawnPoint = PlayerIdToSpawnPointMap.Find(PlayerUniqueID);
	if (SpawnPoint != nullptr)
	{
		return *SpawnPoint;
	}

	AActor* ChosenSpawnPoint = SpawnManager->GetSpawnPointActorByIndex(PlayersSpawned);
	PlayerIdToSpawnPointMap.Add(PlayerUniqueID, ChosenSpawnPoint);

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Spawning player %d at %s."), PlayerUniqueID, *ChosenSpawnPoint->GetActorLocation().ToString());
	
	if (Player->GetIsSimulated())
	{
		AIControlledPlayers.Emplace(ControllerIntegerPair{ Player, PlayersSpawned });
	}

	PlayersSpawned++;

	return ChosenSpawnPoint;
}

void ABenchmarkGymGameMode::OnPlayerDensityFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	PlayerDensity = FCString::Atoi(*FlagValue);
	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("PlayerDensity %d"), PlayerDensity);
}