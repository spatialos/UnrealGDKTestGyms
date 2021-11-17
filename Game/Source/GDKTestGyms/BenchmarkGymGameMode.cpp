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
	const FString ActorMigrationValidMetricName = TEXT("UnrealActorMigration");

	const FString PlayerDensityWorkerFlag = TEXT("player_density");
	const FString BenchmarkPlayerDensityCommandLineKey = TEXT("PlayerDensity=");

	// Returns an array of coordinates. Each coordinate represents the center of a cell in a generated grid.
	// The grid created will abide by these rules if the function returns true:
	//		- Fit inside the dimensions GridMaxWidth, GridMaxHeight.
	//		- Have an odd number of rows and columns.
	//		- Have at minimum MinCells cells.
	//		- All center positions will be CellSize apart.

	bool GenerateGridInArea(const float GridMaxWidth, const float GridMaxHeight, const int32 MinCells, const float CellSize, const FVector& WorldPositon, TArray<FVector>& OutCellCenterPoints)
	{
		if (CellSize == 0.0f)
		{
			UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Trying to generate grid settings using invalid CellSize"));
			return false;
		}

		const int32 MaxRows = FMath::FloorToInt(GridMaxHeight / CellSize);
		const int32 MaxCols = FMath::FloorToInt(GridMaxWidth / CellSize);

		// Ensure we have odd number of rows and cols.
		int32 Rows = MaxRows % 2 == 0 ? FMath::Max(0, MaxRows - 1) : MaxRows;
		int32 Cols = MaxCols % 2 == 0 ? FMath::Max(0, MaxCols - 1) : MaxCols;

		if (Rows * Cols < MinCells)
		{
			UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Area not big enough for MinCells"));
			return false;
		}

		const float GridWidth = Cols * CellSize;
		const float GridHeight = Rows * CellSize;

		const float StartX = WorldPositon.X + (CellSize - GridWidth) / 2.0f;
		const float StartY = WorldPositon.Y + (CellSize - GridHeight) / 2.0f;

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

// --- FSpawnCluster ---

 bool FSpawnCluster::GenerateSpawnPoints()
{
	// Spawn points 3m apart to avoid spawn collision issues.
	TArray<FVector> SpawnPoints;
	const bool bSuccefullyCreatedGrid = GenerateGridInArea(Width, Height, MaxSpawnPoints, MinDistanceBetweenSpawnPoints, WorldPosition, SpawnPoints);
	if (!bSuccefullyCreatedGrid)
	{
		return false;
	}

	// Sort the spawn points by "distance from centre of cluster" ascending.
	// This ensures we iterate from closest to furthest spawn point.
	SpawnPoints.Sort([this](const FVector& Point1, const FVector& Point2)
	{
		const float DistanceSqr1 = FVector::DistSquared(Point1, WorldPosition);
		const float DistanceSqr2 = FVector::DistSquared(Point2, WorldPosition);
		return DistanceSqr2 > DistanceSqr1;
	});

	//Remove any excess spawn points
	SpawnPoints.RemoveAt(MaxSpawnPoints, SpawnPoints.Num() - MaxSpawnPoints);

	SpawnPointActors.Empty();
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
	return true;
}

// --- FSpawnArea ---

 bool FSpawnArea::GenerateSpawnClusters()
{
	TArray<FVector> ClusterPoints;
	const bool bSuccefullyCreatedGrid = GenerateGridInArea(Width, Height, NumClusters, MinDistanceBetweenClusters, WorldPosition, ClusterPoints);
	if (!bSuccefullyCreatedGrid)
	{
		return false;
	}

	// Sort the clusters by "distance from centre of area" ascending.
	// This ensures we iterate from closest to furthest cluster.
	ClusterPoints.Sort([this](const FVector& Point1, const FVector& Point2)
	{
		const float DistanceSqr1 = FVector::DistSquared(Point1, WorldPosition);
		const float DistanceSqr2 = FVector::DistSquared(Point2, WorldPosition);
		return DistanceSqr2 > DistanceSqr1;
	});

	//Remove any excess areas
	ClusterPoints.RemoveAt(NumClusters, ClusterPoints.Num() - NumClusters);

	SpawnPointActors.Empty();
	for (int32 i = 0; i < ClusterPoints.Num() && i < NumClusters; ++i)
	{
		const FVector& ClusterPoint = ClusterPoints[i];
		FSpawnCluster NewSpawnCluster;

		NewSpawnCluster.World = World;
		NewSpawnCluster.WorldPosition = ClusterPoint;
		NewSpawnCluster.Width = MinDistanceBetweenClusters;
		NewSpawnCluster.Height = MinDistanceBetweenClusters;
		NewSpawnCluster.MaxSpawnPoints = MaxSpawnPointsPerCluster;
		NewSpawnCluster.MinDistanceBetweenSpawnPoints = MinDistanceBetweenSpawnPoints;

		if (NewSpawnCluster.GenerateSpawnPoints())
		{
			SpawnPointActors += NewSpawnCluster.GetSpawnPointActors();
		}
		SpawnClusters.Add(NewSpawnCluster);
	}
	return true;
}

AActor* FSpawnArea::GetSpawnPointActorByIndex(const int32 Index) const
{
	const int32 NumSpawnPointActors = SpawnPointActors.Num();
	if (NumSpawnPointActors == 0)
	{
		return nullptr;
	}
	return SpawnPointActors[Index % NumSpawnPointActors];
}

// --- USpawnManager ---

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

void USpawnManager::ForEachArea(TFunctionRef<void(FSpawnArea& ZoneArea)> Predicate, const FSpawnArea::AreaType AreaType /*=FSpawnArea::Type::Count*/)
{
	const bool bAllAreaTypes = AreaType == FSpawnArea::AreaType::Count;
	for (FSpawnArea& SpawnArea : SpawnAreas)
	{
		if (bAllAreaTypes || SpawnArea.Type == AreaType)
		{
			Predicate(SpawnArea);
		}
	}
}

void USpawnManager::GenerateSpawnAreas(const int32 ZoneRows, const int32 ZoneCols, const int32 ZoneWidth, const int32 ZoneHeight,
	const int32 ZoneClusters, const int32 BoundaryClusters,
	const int32 MaxSpawnPointsPerCluster, const float MinDistanceBetweenClusters, const float MinDistanceBetweenSpawnPoints)
{
	const float StartX = ZoneWidth * (1 - ZoneCols) / 2.0f;
	const float StartY = ZoneHeight * (1 - ZoneRows) / 2.0f;
	const int32 SpawnAreaRows = ZoneRows * 2 - 1;
	const int32 SpawnAreaCols = ZoneCols * 2 - 1;
	const int32 SpawnAreaWidth = ZoneWidth - MinDistanceBetweenClusters;
	const int32 SpawnAreaHeight = ZoneHeight - MinDistanceBetweenClusters;

	const int32 NumZones = ZoneRows * ZoneCols;
	const int32 NumBoundaries = ZoneRows * (ZoneCols - 1) + ZoneCols * (ZoneRows - 1);

	UWorld* World = GetWorld();

	int32 ZoneClustersToAdd = ZoneClusters;
	int32 BoundaryClustersToAdd = BoundaryClusters;

	int32 NumZonesLeftToProcess = NumZones;
	int32 NumBoundariesToProcess = NumBoundaries;

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

			int32& ClustersToAdd = bIsZone ? ZoneClustersToAdd : BoundaryClustersToAdd;
			int32& AreasToProcess = bIsZone ? NumZonesLeftToProcess : NumBoundariesToProcess;
			if (ClustersToAdd <= 0)
			{
				// Don't create new areas if we have already added enough clusters for area type.
				AreasToProcess--;
				continue;
			}

			FSpawnArea NewSpawnArea;

			NewSpawnArea.World = World;
			NewSpawnArea.Type = bIsZone ? FSpawnArea::AreaType::Zone : FSpawnArea::AreaType::Boundary;

			const float X = StartX + Col * ZoneWidth / 2.0f;
			const float Y = StartY + Row * ZoneHeight / 2.0f;
			NewSpawnArea.WorldPosition = { X, Y, 0.0f };

			NewSpawnArea.Width = bIsZone ? SpawnAreaWidth : bZoneRow ? MinDistanceBetweenClusters : SpawnAreaWidth;
			NewSpawnArea.Height = bIsZone ? SpawnAreaHeight : bZoneCol ? MinDistanceBetweenClusters : SpawnAreaHeight;

			checkf(AreasToProcess != 0.0f, TEXT("Spawning point logic has gone wrong!"));
			const int32 NumAreaClusters = FMath::CeilToInt(ClustersToAdd / static_cast<float>(AreasToProcess));
			NewSpawnArea.NumClusters = NumAreaClusters;

			NewSpawnArea.MaxSpawnPointsPerCluster = MaxSpawnPointsPerCluster;
			NewSpawnArea.MinDistanceBetweenClusters = MinDistanceBetweenClusters;
			NewSpawnArea.MinDistanceBetweenSpawnPoints = MinDistanceBetweenSpawnPoints;

			if (NewSpawnArea.GenerateSpawnClusters())
			{
				SpawnPointActors += NewSpawnArea.GetSpawnPointActors();
			}

			SpawnAreas.Add(NewSpawnArea);

			ClustersToAdd -= NumAreaClusters;
			AreasToProcess--;
		}
	}
}

// --- ABenchmarkGymGameMode ---

ABenchmarkGymGameMode::ABenchmarkGymGameMode()
	: DistBetweenSpawnPoints(300.0f)
	, PercentageSpawnPointsOnWorkerBoundaries(0.05f)
	, bHasCreatedSpawnPoints(false)
	, PlayerDensity(0) // PlayerDensity is invalid until set via command line arg or worker flag.
	, PlayersSpawned(0)
	, NPCSToSpawn(0)
	, bIsUsingZoning(false)
	, bHasActorMigrationCheckFailed(false)
	, PreviousTickMigration(0)
	, MigrationOfCurrentWorker(0)
	, MigrationCountSeconds(0.0)
	, MigrationWindowSeconds(5 * 60.0f)
	, MinActorMigrationPerSecond(0.0f)
	, ActorMigrationReportTimer(1)
	, ActorMigrationCheckTimer(11 * 60) // 1-minute later then ActorMigrationCheckDelay + MigrationWindowSeconds to make sure all the workers had reported their migration	
	, ActorMigrationCheckDelay(5 * 60)
{
	PrimaryActorTick.bCanEverTick = true;

	const APawn* Pawn = GetDefault<APawn>(SimulatedPawnClass);
	DistBetweenClusters = FMath::Sqrt(Pawn->NetCullDistanceSquared) * 2.2f;
}

void ABenchmarkGymGameMode::BeginPlay()
{
	Super::BeginPlay();
	bIsUsingZoning = USpatialStatics::IsSpatialNetworkingEnabled() && (GetZoningRows() > 1 || GetZoningCols() > 1);
	SpawnManager = NewObject<USpawnManager>(this);

#if WITH_EDITOR
	if (ExpectedPlayers == 0)
	{
		ExpectedPlayers = 4;
	}
	if (PlayerDensity == 0)
	{
		PlayerDensity = 2;
	}
#endif // WITH_EDITOR
}

void ABenchmarkGymGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsUsingZoning)
	{
		TickActorMigration(DeltaSeconds);
	}

	if (HasAuthority())
	{
		TryStartCustomNPCSpawning();
		TickNPCSpawning();
		TickSimPlayerBlackboardValues();
	}
}

void ABenchmarkGymGameMode::TickNPCSpawning()
{
	if (NPCSToSpawn > 0)
	{
		const int32 NPCIndex = TotalNPCs - NPCSToSpawn;
		const AActor* SpawnPoint = SpawnManager->GetSpawnPointActorByIndex(NPCIndex);
		if (SpawnPoint != nullptr)
		{
			const FVector SpawnLocation = SpawnPoint->GetActorLocation();
			if (SpawnNPC(SpawnLocation, NPCRunPoints[NPCIndex % NPCRunPoints.Num()]))
			{
				NPCSToSpawn--;
			}
		}
	}
}

void ABenchmarkGymGameMode::TickSimPlayerBlackboardValues()
{
	for (int32 i = AIControlledPlayers.Num() - 1; i >= 0; i--)
	{
		AController* Controller = AIControlledPlayers[i].Key.Get();
		const int32 InfoIndex = AIControlledPlayers[i].Value;

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

void ABenchmarkGymGameMode::TickActorMigration(float DeltaSeconds)
{
	if (bHasActorMigrationCheckFailed)
	{
		return;
	}

	const int32 AuthActorCount = GetUXAuthActorCount();
	// TODO: UNR-5825 - fix migration metric check
	//MinActorMigrationPerSecond = PercentageSpawnPointsOnWorkerBoundaries * (ExpectedPlayers + TotalNPCs) * 0.02f;
	MinActorMigrationPerSecond = SMALL_NUMBER;

	// This test respects the initial delay timer only for multiworker
	if (ActorMigrationCheckDelay.HasTimerGoneOff())
	{
		// Count how many actors hand over authority in 1 tick
		const int32 Delta = FMath::Abs(AuthActorCount - PreviousTickMigration);
		MigrationDeltaHistory.Enqueue(MigrationDeltaPair(Delta, DeltaSeconds));
		bool bChanged = false;
		if (MigrationCountSeconds > MigrationWindowSeconds)
		{
			MigrationDeltaPair OldestValue;
			if (MigrationDeltaHistory.Dequeue(OldestValue))
			{
				MigrationOfCurrentWorker -= OldestValue.Key;
				MigrationSeconds -= OldestValue.Value;
			}
		}

		MigrationOfCurrentWorker += Delta;
		MigrationSeconds += DeltaSeconds;
		PreviousTickMigration = AuthActorCount;

		if (MigrationCountSeconds > MigrationWindowSeconds)
		{
			if (ActorMigrationReportTimer.HasTimerGoneOff())
			{
				// Only report AverageMigrationOfCurrentWorkerPerSecond to the worker which has authority
				float AverageMigrationOfCurrentWorkerPerSecond = MigrationOfCurrentWorker / MigrationSeconds;
				ReportMigration(GetGameInstance()->GetSpatialWorkerId(), AverageMigrationOfCurrentWorkerPerSecond);
				ActorMigrationReportTimer.SetTimer(1);
			}

			if (HasAuthority() && ActorMigrationCheckTimer.HasTimerGoneOff())
			{
				float Migration = 0.0f;
				for (const auto& KeyValue : MapWorkerActorMigration)
				{
					Migration += KeyValue.Value;
				}
				if (Migration < MinActorMigrationPerSecond)
				{
					bHasActorMigrationCheckFailed = true;
					NFR_LOG(LogBenchmarkGymGameModeBase, Error, TEXT("%s: Actor migration check failed. Migration=%.8f MinActorMigrationPerSecond=%.8f MigrationExactlyWindowSeconds=%.8f"),
						*NFRFailureString, Migration, MinActorMigrationPerSecond, MigrationSeconds);
				}
				else
				{
					// Reset timer for next check after 10s
					ActorMigrationCheckTimer.SetTimer(10);
					UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Actor migration check TotalMigrations=%.8f MinActorMigrationPerSecond=%.8f MigrationExactlyWindowSeconds=%.8f"),
						Migration, MinActorMigrationPerSecond, MigrationSeconds);
				}
			}
		}
		MigrationCountSeconds += DeltaSeconds;
	}
}

void ABenchmarkGymGameMode::AddSpatialMetrics(USpatialMetrics* SpatialMetrics)
{
	Super::AddSpatialMetrics(SpatialMetrics);

	if (HasAuthority())
	{
		UserSuppliedMetric Delegate;
		Delegate.BindUObject(this, &ABenchmarkGymGameMode::GetTotalMigrationValid);
		SpatialMetrics->SetCustomMetric(ActorMigrationValidMetricName, Delegate);
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
	const float MinDistanceSq = 500.f * 500.f;	// Move To threshold used in behaviour tree
	{
		FRandomStream PlayerStream;
		PlayerStream.Initialize(FCrc::MemCrc32(&ExpectedPlayers, sizeof(ExpectedPlayers))); // Ensure we can do deterministic runs
		for (int32 i = 0; i < ExpectedPlayers; i++)
		{
			FVector PointA = PlayerStream.VRand()*RoamRadius;
			FVector PointB = PlayerStream.VRand()*RoamRadius;
			PointA.Z = PointB.Z = 0.0f;
			if (FVector::DistSquared2D(PointA, PointB) < MinDistanceSq)
			{
				PointA *= -1.f;
			}
			PlayerRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
		}
	}
	{
		FRandomStream NPCStream;
		NPCStream.Initialize(FCrc::MemCrc32(&TotalNPCs, sizeof(TotalNPCs)));
		for (int32 i = 0; i < TotalNPCs; i++)
		{
			FVector PointA = NPCStream.VRand()*RoamRadius;
			FVector PointB = NPCStream.VRand()*RoamRadius;
			PointA.Z = PointB.Z = 0.0f;
			if (FVector::DistSquared2D(PointA, PointB) < MinDistanceSq)
			{
				PointA *= -1.f;
			}
			NPCRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
		}
	}
}

void ABenchmarkGymGameMode::TryStartCustomNPCSpawning()
{
	if (bHasCreatedSpawnPoints || ExpectedPlayers == 0 || PlayerDensity == 0)
	{
		return;
	}

	StartCustomNPCSpawning();
	bHasCreatedSpawnPoints = true;
}

void ABenchmarkGymGameMode::StartCustomNPCSpawning()
{
	GenerateSpawnPoints();
	GenerateTestScenarioLocations();

	const int32 NumSpawnPoints = SpawnManager->GetNumSpawnPoints();
	if (NumSpawnPoints < ExpectedPlayers) // SpawnPoints can be rounded up if ExpectedPlayers % NumClusters != 0
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), NumSpawnPoints, ExpectedPlayers);
	}
}

void ABenchmarkGymGameMode::BuildExpectedActorCounts()
{
	Super::BuildExpectedActorCounts();

	const int32 TotalDropCubes = TotalNPCs + ExpectedPlayers;
	const int32 DropCubeCountVariance = FMath::CeilToInt(TotalDropCubes * 0.1f) + 2;
	AddExpectedActorCount(DropCubeClass, TotalDropCubes - DropCubeCountVariance, TotalDropCubes + DropCubeCountVariance);
}

void ABenchmarkGymGameMode::GenerateSpawnPoints()
{
	const int32 Rows = GetZoningRows();
	const int32 Cols = GetZoningCols();
	const float Width = GetZoneWidth();
	const float Height = GetZoneHeight();

	const int32 NumClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));
	const int32 BoundaryClusters = Rows > 1 || Cols > 1 ? FMath::CeilToInt(NumClusters * PercentageSpawnPointsOnWorkerBoundaries) : 0;
	const int32 ZoneClusters = NumClusters - BoundaryClusters;

	SpawnManager->GenerateSpawnAreas(Rows, Cols, Width, Height, ZoneClusters, BoundaryClusters, PlayerDensity, DistBetweenClusters, DistBetweenSpawnPoints);
}

void ABenchmarkGymGameMode::SpawnNPCs(int NumNPCs)
{
	NPCSToSpawn = NumNPCs;
}

bool ABenchmarkGymGameMode::SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogBenchmarkGymGameMode, Error, TEXT("Error spawning NPC, World is null"));
		return false;
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

	if (NPCSpawner == nullptr || !NPCSpawner->IsActorReady())
	{
		UE_LOG(LogBenchmarkGymGameMode, Warning, TEXT("Could not spawn NPC. Will retry."));
		return false;
	}

	NPCSpawner->CrossServerSpawn(NPCClass, SpawnLocation, BlackboardValues);
	return true;
}

void ABenchmarkGymGameMode::ReportMigration_Implementation(const FString& WorkerID, const float Migration)
{
	if (HasAuthority())
	{
		MapWorkerActorMigration.Emplace(WorkerID, Migration);
	}
}

AActor* ABenchmarkGymGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	if (SpawnManager->GetNumSpawnPoints() == 0)
	{
		return nullptr;
	}

	if (Player == nullptr) // Work around for load balancing passing nullptr Player
	{
		return SpawnManager->GetSpawnPointActorByIndex(PlayersSpawned);
	}

	AActor* ChosenSpawnPoint = SpawnManager->GetSpawnPointActorByIndex(PlayersSpawned);

	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("Spawning player %d at %s."), PlayersSpawned, *ChosenSpawnPoint->GetActorLocation().ToString());
	
	if (Player->GetIsSimulated())
	{
		AIControlledPlayers.Emplace(ControllerIntegerPair{ Player, PlayersSpawned });
	}

	PlayersSpawned++;

	return ChosenSpawnPoint;
}

void ABenchmarkGymGameMode::OnTotalNPCsUpdated_Implementation(int32 Value)
{
	Super::OnTotalNPCsUpdated_Implementation(Value);
	SpawnNPCs(Value);
}

void ABenchmarkGymGameMode::OnPlayerDensityFlagUpdate(const FString& FlagName, const FString& FlagValue)
{
	PlayerDensity = FCString::Atoi(*FlagValue);
	UE_LOG(LogBenchmarkGymGameMode, Log, TEXT("PlayerDensity %d"), PlayerDensity);
}