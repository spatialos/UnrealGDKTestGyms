// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "AUptimeGameMode.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BenchmarkGymNPCSpawner.h"
#include "DeterministicBlackboardValues.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "GeneralProjectSettings.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"

DEFINE_LOG_CATEGORY(LogUptimeGymGameMode);

namespace
{
	const FString UptimePlayerDensityWorkerFlag = TEXT("player_density");
	const FString UptimePlayerDensityCommandLineKey = TEXT("PlayerDensity=");

	const FString UptimeSpawnColsWorkerFlag = TEXT("spawn_cols");
	const FString UptimeSpawnColsCommandLineKey = TEXT("-SpawnCols=");
	
	const FString UptimeSpawnRowsWorkerFlag = TEXT("spawn_rows");
	const FString UptimeSpawnRowsCommandLineKey = TEXT("-SpawnRows=");

	const FString UptimeWorldWidthWorkerFlag = TEXT("zone_width");
	const FString UptimeWorldWidthCommandLineKey = TEXT("-ZoneWidth=");

	const FString UptimeWorldHeightWorkerFlag = TEXT("zone_height");
	const FString UptimeWorldHeightCommandLineKey = TEXT("-ZoneHeight=");

	const FString UptimeEgressSizeWorkerFlag = TEXT("egress_test_size");
	const FString UptimeEgressSizeCommandLineKey = TEXT("-EgressTestSize=");
	
	const FString UptimeEgressFrequencyWorkerFlag = TEXT("egress_test_frequency");
	const FString UptimeEgressFrequencyCommandLineKey = TEXT("-EgressTestFrequency=");
	
	const FString UptimeCrossServerSizeWorkerFlag = TEXT("cross_server_size");
	const FString UptimeCrossServerSizeCommandLineKey = TEXT("-CrossServerSize=");

	const FString UptimeCrossServerFrequencyWorkerFlag = TEXT("cross_server_frequency");
	const FString UptimeCrossServerFrequencyCommandLineKey = TEXT("-CrossServerFrequency=");
} // anonymous namespace

AUptimeGameMode::AUptimeGameMode()
	: bInitializedCustomSpawnParameters(false)
	, SpawnCols(2)
	, SpawnRows(1)
	, ZoneWidth(1000000.0f)
	, ZoneHeight(1000000.0f)
	, TestDataSize(0)
	, TestDataFrequency(0)
	, CrossServerSize(0)
	, CrossServerFrequency(0)
	, NumPlayerClusters(1)
	, PlayersSpawned(0)
	, NPCSToSpawn(0)
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<AActor> DropCubeClassFinder(TEXT("/Game/Benchmark/DropCube_BP"));
	DropCubeClass = DropCubeClassFinder.Class;
}

void AUptimeGameMode::GenerateTestScenarioLocations()
{
	constexpr float RoamRadius = 7500.0f; // Set to half the NetCullDistance
	{
		FRandomStream PlayerStream;
		PlayerStream.Initialize(FCrc::MemCrc32(&ExpectedPlayers, sizeof(ExpectedPlayers))); // Ensure we can do deterministic runs
		for (int i = 0; i < ExpectedPlayers; i++)
		{
			FVector PointA = PlayerStream.VRand() * RoamRadius;
			FVector PointB = PlayerStream.VRand() * RoamRadius;
			PointA.Z = PointB.Z = 0.0f;
			PlayerRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
		}
	}
	{
		FRandomStream NPCStream;
		NPCStream.Initialize(FCrc::MemCrc32(&TotalNPCs, sizeof(TotalNPCs)));
		for (int i = 0; i < TotalNPCs; ++i)
		{
			FVector PointA = NPCStream.VRand() * RoamRadius;
			FVector PointB = NPCStream.VRand() * RoamRadius;
			PointA.Z = PointB.Z = 0.0f;
			NPCRunPoints.Emplace(FBlackboardValues{ PointA, PointB });
		}
	}
}

void AUptimeGameMode::CheckCmdLineParameters()
{
	if (bInitializedCustomSpawnParameters)
	{
		return;
	}

	ParsePassedValues();
	StartCustomNPCSpawning();

	bInitializedCustomSpawnParameters = true;
}

void AUptimeGameMode::StartCustomNPCSpawning()
{
	ClearExistingSpawnPoints();
	GenerateSpawnPointClusters(NumPlayerClusters);

	if (SpawnPoints.Num() < ExpectedPlayers) // SpawnPoints can be rounded up if ExpectedPlayers % NumClusters != 0
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), SpawnPoints.Num(), ExpectedPlayers);
	}

	GenerateTestScenarioLocations();

	SpawnNPCs(TotalNPCs);
	SpawnCrossServerActors(GetNumWorkers());
}

void AUptimeGameMode::Tick(float DeltaSeconds)
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

void AUptimeGameMode::ParsePassedValues()
{
	Super::ParsePassedValues();

	PlayerDensity = ExpectedPlayers;

	const FString& CommandLine = FCommandLine::Get();
	if (FParse::Param(*CommandLine, *ReadFromCommandLineKey))
	{
		FParse::Value(*CommandLine, *UptimePlayerDensityCommandLineKey, PlayerDensity);
		FParse::Value(*CommandLine, *UptimeSpawnColsCommandLineKey, SpawnCols);
		FParse::Value(*CommandLine, *UptimeSpawnRowsCommandLineKey, SpawnRows);
		FParse::Value(*CommandLine, *UptimeWorldWidthCommandLineKey, ZoneHeight);
		FParse::Value(*CommandLine, *UptimeWorldHeightCommandLineKey, ZoneWidth);
		FParse::Value(*CommandLine, *UptimeEgressSizeCommandLineKey, TestDataSize);
		FParse::Value(*CommandLine, *UptimeEgressFrequencyCommandLineKey, TestDataFrequency);
		FParse::Value(*CommandLine, *UptimeCrossServerSizeCommandLineKey, CrossServerSize);
		FParse::Value(*CommandLine, *UptimeCrossServerFrequencyCommandLineKey, CrossServerFrequency);
	}
	else if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		UE_LOG(LogBenchmarkGymGameModeBase, Log, TEXT("Using worker flags to load custom spawning parameters."));

		USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver());
		if (ensure(NetDriver != nullptr))
		{
			const USpatialWorkerFlags* SpatialWorkerFlags = NetDriver->SpatialWorkerFlags;
			if (ensure(SpatialWorkerFlags != nullptr))
			{
				FString PlayerDensityString;
				if (SpatialWorkerFlags->GetWorkerFlag(UptimePlayerDensityWorkerFlag, PlayerDensityString))
				{
					PlayerDensity = FCString::Atoi(*PlayerDensityString);
				}
			}
		}

	}
	NumPlayerClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));

	UE_LOG(LogUptimeGymGameMode, Log, TEXT("Density %d, Clusters %d, SpawnCols %d, SpawnRows %d, ZoneHeight %d, ZoneWidth %d, TestDataSize %d, TestDataFrequency %d, CrossServerSize %d, CrossServerFrequency %d"), PlayerDensity, NumPlayerClusters, SpawnCols, SpawnRows, ZoneHeight, ZoneWidth, TestDataSize, TestDataFrequency, CrossServerSize, CrossServerFrequency);
}

void AUptimeGameMode::OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue)
{
	Super::OnAnyWorkerFlagUpdated(FlagName, FlagValue);
	if (FlagName == UptimePlayerDensityWorkerFlag)
	{
		PlayerDensity = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == UptimeSpawnColsWorkerFlag)
	{
		SpawnCols = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == UptimeSpawnRowsWorkerFlag)
	{
		SpawnRows = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == UptimeWorldWidthWorkerFlag)
	{
		ZoneWidth = FCString::Atof(*FlagValue);
	}
	else if (FlagName == UptimeWorldHeightWorkerFlag)
	{
		ZoneHeight = FCString::Atof(*FlagValue);
	}
	else if (FlagName == UptimeEgressSizeWorkerFlag)
	{
		TestDataSize = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == UptimeEgressFrequencyWorkerFlag)
	{
		TestDataFrequency = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == UptimeCrossServerSizeWorkerFlag)
	{
		CrossServerSize = FCString::Atoi(*FlagValue);
	}
	else if (FlagName == UptimeCrossServerFrequencyWorkerFlag)
	{
		CrossServerFrequency = FCString::Atoi(*FlagValue);
	}
}

void AUptimeGameMode::BuildExpectedActorCounts()
{
	Super::BuildExpectedActorCounts();

	const int32 TotalDropCubes = TotalNPCs + ExpectedPlayers;
	const int32 DropCubeCountVariance = FMath::CeilToInt(TotalDropCubes * 0.1f) + 2;
	AddExpectedActorCount(DropCubeClass, TotalDropCubes - DropCubeCountVariance, TotalDropCubes + DropCubeCountVariance);
}

void AUptimeGameMode::ClearExistingSpawnPoints()
{
	for (AActor* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->Destroy();
	}
	SpawnPoints.Reset();
	PlayerIdToSpawnPointMap.Reset();
}

void AUptimeGameMode::GenerateSpawnPointClusters(int NumClusters)
{
	const int DistBetweenClusterCenters = 40000; // 400 meters, in Unreal units.
	//spawn zones are flexible as rows*cols now
	const float DistBetweenRows = ZoneHeight / SpawnRows;
	const float DistBetweenCols = ZoneWidth / SpawnCols;
	const float StartingX = -ZoneWidth / 2;
	const float StartingY = -ZoneHeight / 2;
	const int32 SpawnZones = SpawnCols * SpawnRows;

	int ClustersPerWorker = FMath::CeilToInt(NumClusters / static_cast<float>(SpawnZones));
	auto CurRows = 0;
	auto CurCols = 0;
	for (auto i = 0; i < SpawnZones; ++i)
	{
		int ClusterCount = FMath::Min(ClustersPerWorker, NumClusters);
		NumClusters -= ClusterCount;
		int NumRows, NumCols, MinRelativeX, MinRelativeY;
		GenerateGridSettings(DistBetweenClusterCenters, ClusterCount, NumRows, NumCols, MinRelativeX, MinRelativeY);

		//Adjust the lefthand side of the grid to so that the grid is centered in the zone
		MinRelativeX = FMath::RoundToInt(MinRelativeX + StartingX + (CurCols * DistBetweenCols) + (DistBetweenCols / 2.0f));
		MinRelativeY = FMath::RoundToInt(MinRelativeY + StartingY + (CurRows * DistBetweenRows) + (DistBetweenRows / 2.0f));
		++CurCols;

		UE_LOG(LogUptimeGymGameMode, Log, TEXT("Creating player cluster grid of %d rows by %d columns from location: %d , %d"), NumRows, NumCols, MinRelativeX, MinRelativeY);
		for (int j = 0; j < ClusterCount; ++j)
		{
			const int Row = j % NumRows;
			const int Col = j / NumRows;

			const int ClusterCenterX = MinRelativeX + Col * DistBetweenClusterCenters;
			const int ClusterCenterY = MinRelativeY + Row * DistBetweenClusterCenters;
			//Because GridBaseLBStrategy swaps X and Y, we will swap them here so that we're aligned
			GenerateSpawnPoints(ClusterCenterY, ClusterCenterX, PlayerDensity);
		}
		if (CurCols % SpawnCols == 0)
		{
			CurCols = 0;
			++CurRows;
		}
	}
}

void AUptimeGameMode::GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY)
{
	if (NumPoints <= 0)
	{
		UE_LOG(LogUptimeGymGameMode, Warning, TEXT("Generating grid settings with non-postive number of points (%d)"), NumPoints);
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

void AUptimeGameMode::GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum)
{
	// Spawn in the air above terrain obstacles (Unreal units).
	const int Z = 300;

	const int DistBetweenSpawnPoints = 300; // In Unreal units.
	int NumRows, NumCols, MinRelativeX, MinRelativeY;
	GenerateGridSettings(DistBetweenSpawnPoints, SpawnPointsNum, NumRows, NumCols, MinRelativeX, MinRelativeY);

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Cannot spawn spawnpoints, world is null"));
		return;
	}

	for (int i = 0; i < SpawnPointsNum; ++i)
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
		UE_LOG(LogUptimeGymGameMode, Log, TEXT("Creating a new PlayerStart at location %s."), *SpawnLocation.ToString());
		SpawnPoints.Add(World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnInfo));
	}
}

void AUptimeGameMode::SpawnNPCs(int NumNPCs)
{
	NPCSToSpawn = NumNPCs;
}

void AUptimeGameMode::SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Error spawning NPC, World is null"));
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
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Failed to find NPCSpawner."));
	}
}

AActor* AUptimeGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
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

	UE_LOG(LogUptimeGymGameMode, Log, TEXT("Spawning player %d at %s."), PlayerUniqueID, *ChosenSpawnPoint->GetActorLocation().ToString());

	if (Player->GetIsSimulated())
	{
		AIControlledPlayers.Emplace(ControllerIntegerPair{ Player, PlayersSpawned });
	}

	PlayersSpawned++;

	return ChosenSpawnPoint;
}

void AUptimeGameMode::SpawnCrossServerActors(int32 CrossServerPointNum)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Error spawning, World is null"));
		return;
	}

	TArray<FVector> Locations = GenerateCrossServerLoaction();
	auto SizeOfLocations = Locations.Num();
	for (auto i = 0; i < SizeOfLocations; ++i)
	{
		AUptimeCrossServerBeacon* Beacon = World->SpawnActor<AUptimeCrossServerBeacon>(CrossServerClass, Locations[i], FRotator::ZeroRotator, FActorSpawnParameters());
		checkf(Beacon, TEXT("Beacon failed to spawn at %s"), *Locations[i].ToString());

		SetCrossServerWorkerFlags(Beacon);
	}
}

TArray<FVector> AUptimeGameMode::GenerateCrossServerLoaction()
{
	const float DistBetweenRows = ZoneHeight / SpawnRows;
	const float DistBetweenCols = ZoneWidth / SpawnCols;
	float StartingX = -(SpawnCols - 1) * ZoneWidth / 2 / SpawnCols;
	float StartingY = -(SpawnRows - 1) * ZoneHeight / 2 / SpawnRows;
	TArray<FVector> Locations;
	const int Z = 300;
	for (auto i = 0; i < SpawnCols; ++i)
	{
		auto TempStartingY = StartingY;
		for (auto j = 0; j < SpawnRows; ++j)
		{
			const int Y = TempStartingY;
			const int X = StartingX;
			TempStartingY += DistBetweenRows;
			FVector Location = FVector(X, Y, Z);
			Locations.Add(Location);
		}
		StartingX += DistBetweenCols;
	}
	return Locations;
}

void AUptimeGameMode::SetCrossServerWorkerFlags(AUptimeCrossServerBeacon* Beacon) const
{
	Beacon->SetCrossServerSize(CrossServerSize);
	Beacon->SetCrossServerFrequency(CrossServerFrequency);
	UE_LOG(LogUptimeGymGameMode, Log, TEXT("cross server size:%d,cross server frequency:%d"), CrossServerSize, CrossServerFrequency);
}
