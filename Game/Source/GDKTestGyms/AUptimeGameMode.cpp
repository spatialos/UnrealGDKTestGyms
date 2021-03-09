// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "AUptimeGameMode.h"

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

DEFINE_LOG_CATEGORY(LogUptimeGymGameMode);

namespace
{
	const FString PlayerDensityWorkerFlag = TEXT("player_density");
	const float PercentageSpawnpointsOnWorkerBoundary = 0.25f;
} // anonymous namespace

AUptimeGameMode::AUptimeGameMode()
	: bInitializedCustomSpawnParameters(false)
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
		for (int i = 0; i < TotalNPCs; i++)
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
		FParse::Value(*CommandLine, TEXT("PlayerDensity="), PlayerDensity);
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
				if (SpatialWorkerFlags->GetWorkerFlag(PlayerDensityWorkerFlag, PlayerDensityString))
				{
					PlayerDensity = FCString::Atoi(*PlayerDensityString);
				}
			}
		}

	}
	NumPlayerClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));

	UE_LOG(LogUptimeGymGameMode, Log, TEXT("Density %d, Clusters %d"), PlayerDensity, NumPlayerClusters);
}

void AUptimeGameMode::OnAnyWorkerFlagUpdated(const FString& FlagName, const FString& FlagValue)
{
	Super::OnAnyWorkerFlagUpdated(FlagName, FlagValue);
	if (FlagName == PlayerDensityWorkerFlag)
	{
		PlayerDensity = FCString::Atoi(*FlagValue);
	}
}

void AUptimeGameMode::BuildExpectedActorCounts()
{
	Super::BuildExpectedActorCounts();

	const int32 TotalDropCubes = TotalNPCs + ExpectedPlayers;
	const int32 DropCubeCountVariance = FMath::CeilToInt(TotalDropCubes * 0.1f) + 2;
	AddExpectedActorCount(DropCubeClass, TotalDropCubes, DropCubeCountVariance);
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

void AUptimeGameMode::GenerateRowBoundaries(float StartingX, float StartingY, float HalfDistBetweenPoints, TArray<FVector2D>& Boundaries, int& BoudariesIndex)
{
	auto PointsNum = BoudariesIndex + 5;
	for (; BoudariesIndex < PointsNum; ++BoudariesIndex)
	{
		Boundaries[BoudariesIndex] = FVector2D(StartingX, StartingY);
		StartingX += HalfDistBetweenPoints;
	}
}

void AUptimeGameMode::GenerateColBoundaries(float StartingX, float StartingY, float DistBetweenPoints, TArray<FVector2D>& Boundaries, int& BoundariesIndex)
{
	auto PointsNum = BoundariesIndex + 3;
	for (; BoundariesIndex < PointsNum; ++BoundariesIndex)
	{
		Boundaries[BoundariesIndex] = FVector2D(StartingX, StartingY);
		StartingY += DistBetweenPoints;
	}
}

void AUptimeGameMode::GenerateCenterBoundaries(float FixedPos, TArray<FVector2D>& Boundaries, int& BoundariesIndex)
{
	auto ContraryFixedPos = -FixedPos;
	Boundaries[BoundariesIndex++] = FVector2D(FixedPos, FixedPos);
	Boundaries[BoundariesIndex++] = FVector2D(FixedPos, ContraryFixedPos);
	Boundaries[BoundariesIndex++] = FVector2D(ContraryFixedPos, ContraryFixedPos);
	Boundaries[BoundariesIndex++] = FVector2D(ContraryFixedPos, FixedPos);
}

void AUptimeGameMode::GenerateAllCenterBoundaries(int32 SpawnZones, float Starting, float DistBetweenZones, TArray<FVector2D>& Boundaries, int32& BoundariesIndex)
{
	auto TempStartingX = Starting;
	for (auto i = 0; i < SpawnZones; ++i)
	{
		auto TempStartingY = Starting;
		for (auto j = 0; j < SpawnZones; ++j)
		{
			Boundaries.Add(FVector2D(TempStartingX, TempStartingY));
			++BoundariesIndex;
			TempStartingY += DistBetweenZones;
		}
		TempStartingX += DistBetweenZones;
	}
}

void AUptimeGameMode::GenerateSpawnPointClusters(int NumClusters)
{
	//spawn zones are fixed as 9,so the row and col changed to 3*3 
	const int32 SpawnZones = 3;
	// We use a fixed size 2km
	const float ZoneWidthAndLength = 200000.0f / SpawnZones;

	TArray<FVector2D> Boundaries;
	int32 BoudariesIndex = 0;
	const float Starting = -200000.0f / 3;
	GenerateAllCenterBoundaries(SpawnZones, Starting, ZoneWidthAndLength, Boundaries, BoudariesIndex);
	
	const float Z = 300.0f;

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogUptimeGymGameMode, Error, TEXT("Cannot spawn spawnpoints, world is null"));
		return; 
	}

	while (--BoudariesIndex >= 0)
	{
		float X = Boundaries[BoudariesIndex].X;
		float Y = Boundaries[BoudariesIndex].Y;

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
