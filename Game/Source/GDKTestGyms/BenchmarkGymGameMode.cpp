// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymGameMode.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DeterministicBlackboardValues.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "Interop/SpatialWorkerFlags.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"

DEFINE_LOG_CATEGORY(LogBenchmarkGym);

ABenchmarkGymGameMode::ABenchmarkGymGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/Characters/PlayerCharacter_BP"));
	DefaultPawnClass = PawnClass.Class;
	PlayerControllerClass = APlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> NPCBPClass(TEXT("/Game/Characters/SimulatedPlayers/BenchmarkNPC_BP"));
	if (NPCBPClass.Class != NULL) 
	{
		NPCPawnClass = NPCBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APawn> SimulatedBPPawnClass(TEXT("/Game/Characters/SimulatedPlayers/SimulatedPlayerCharacter_BP"));
	if (SimulatedBPPawnClass.Class != NULL)
	{
		SimulatedPawnClass = SimulatedBPPawnClass.Class;
	}

	bHasUpdatedMaxActorsToReplicate = false;
	bInitializedCustomSpawnParameters = false;

	ExpectedPlayers = 1;
	TotalNPCs = 0;
	NumPlayerClusters = 4;
	PlayersSpawned = 0;

	// Seamless Travel is not currently supported in SpatialOS [UNR-897]
	bUseSeamlessTravel = false;

	NPCSToSpawn = 0;
	SecondsTillPlayerCheck = 15.0f * 60.0f;
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

	if (ShouldUseCustomSpawning())
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Enabling custom density spawning."));
		ParsePassedValues();
		ClearExistingSpawnPoints();

		SpawnPoints.Reset();
		GenerateSpawnPointClusters(NumPlayerClusters);

		if (SpawnPoints.Num() != ExpectedPlayers) 
		{
			UE_LOG(LogBenchmarkGym, Error, TEXT("Error creating spawnpoints, number of created spawn points (%d) does not equal total players (%d)"), SpawnPoints.Num(), ExpectedPlayers);
		}

		GenerateTestScenarioLocations();

		SpawnNPCs(TotalNPCs);
	}
	else
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Custom density spawning disabled."));
	}

	bInitializedCustomSpawnParameters = true;
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

		if (SecondsTillPlayerCheck > 0.0f)
		{
			SecondsTillPlayerCheck -= DeltaSeconds;
			if (SecondsTillPlayerCheck <= 0.0f)
			{
				if (GetNumPlayers() != ExpectedPlayers)
				{
					// This log is used by the NFR pipeline to indicate if a client failed to connect
					UE_LOG(LogBenchmarkGym, Error, TEXT("A client connection was dropped. Expected %d, got %d"), ExpectedPlayers, GetNumPlayers());
				}
				else
				{
					// Useful for NFR log inspection
					UE_LOG(LogBenchmarkGym, Log, TEXT("All clients successfully connected. Expected %d, got %d"), ExpectedPlayers, GetNumPlayers());
				}
			}
		}

		for (int i = AIControlledPlayers.Num() - 1; i >= 0; i--)
		{
			if (AIControlledPlayers[i].Controller == nullptr)
			{
				AIControlledPlayers.RemoveAtSwap(i);
				continue;
			}

			ACharacter* Character = AIControlledPlayers[i].Controller->GetCharacter();
			if (Character == nullptr)
			{
				AIControlledPlayers.RemoveAtSwap(i);
				continue;
			}

			int InfoIndex = AIControlledPlayers[i].Index;
			if (UDeterministicBlackboardValues* Blackboard = Cast<UDeterministicBlackboardValues>(Character->FindComponentByClass(UDeterministicBlackboardValues::StaticClass())))
			{
				const FBlackboardValues& Points = PlayerRunPoints[InfoIndex % PlayerRunPoints.Num()];
				Blackboard->ClientSetBlackboardAILocations(Points);
				AIControlledPlayers.RemoveAtSwap(i);
			}
		}
	}
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
	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetNetDriver());
	if (FParse::Param(FCommandLine::Get(), TEXT("OverrideSpawning")))
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Found OverrideSpawning in command line args, worker flags for custom spawning will be ignored."));
		FParse::Value(FCommandLine::Get(), TEXT("TotalPlayers="), ExpectedPlayers);
		// Set default value of PlayerDensity equal to TotalPlayers. Will be overwritten if PlayerDensity option is specified.
		PlayerDensity = ExpectedPlayers;
		FParse::Value(FCommandLine::Get(), TEXT("PlayerDensity="), PlayerDensity);
		FParse::Value(FCommandLine::Get(), TEXT("TotalNPCs="), TotalNPCs);
	}
	else
	{
		UE_LOG(LogBenchmarkGym, Log, TEXT("Using worker flags to load custom spawning parameters."));
		FString TotalPlayersString, PlayerDensityString, TotalNPCsString;
		if (NetDriver != nullptr && NetDriver->SpatialWorkerFlags != nullptr && NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("total_players"), TotalPlayersString))
		{
			ExpectedPlayers = FCString::Atoi(*TotalPlayersString);
		}
		// Set default value of PlayerDensity equal to TotalPlayers. Will be overwritten if PlayerDensity option is specified.
		PlayerDensity = ExpectedPlayers;
		if (NetDriver != nullptr && NetDriver->SpatialWorkerFlags != nullptr)
		{
			if (NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("player_density"), PlayerDensityString))
			{
				PlayerDensity = FCString::Atoi(*PlayerDensityString);
			}
			if (NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("total_npcs"), TotalNPCsString))
			{
				TotalNPCs = FCString::Atoi(*TotalNPCsString);
			}
		}
	}
	NumPlayerClusters = FMath::CeilToInt(ExpectedPlayers / static_cast<float>(PlayerDensity));
}

void ABenchmarkGymGameMode::ClearExistingSpawnPoints()
{
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), SpawnPoints);
	for (AActor* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->Destroy();
	}
}

void ABenchmarkGymGameMode::GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& OutNumRows, int& OutNumCols, int& OutMinRelativeX, int& OutMinRelativeY)
{
	if (NumPoints <= 0)
	{
		UE_LOG(LogBenchmarkGym, Warning, TEXT("Generating grid settings with non-postive number of points (%d)"), NumPoints);
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

void ABenchmarkGymGameMode::Logout(AController* Controller)
{
	if(SecondsTillPlayerCheck > 0.0f)
	{
		// This log is used by the NFR pipeline to indicate if a client disconnected
		UE_LOG(LogBenchmarkGym, Error, TEXT("A client connection was dropped. Expected %d, got %d, Controller=%s"), ExpectedPlayers, GetNumPlayers(), *Controller->GetName());
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
		UE_LOG(LogBenchmarkGym, Error, TEXT("Error spawning NPC, World is null"));
		return;
	}

	if (NPCPawnClass == nullptr)
	{
		UE_LOG(LogBenchmarkGym, Error, TEXT("Error spawning NPC, NPCPawnClass is not set."));
		return;
	}

	FVector FixedSpawnLocation = SpawnLocation*0.9f + BlackboardValues.TargetAValue*0.1f; // Move slightly away from the spawn location
	UE_LOG(LogBenchmarkGym, Log, TEXT("Spawning NPC at %s"), *SpawnLocation.ToString());
	FActorSpawnParameters SpawnInfo{};
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Pawn = World->SpawnActor<APawn>(NPCPawnClass->GetDefaultObject()->GetClass(), FixedSpawnLocation, FRotator::ZeroRotator, SpawnInfo);
	checkf(Pawn, TEXT("Pawn failed to spawn at %s"), *FixedSpawnLocation.ToString());
	
	UDeterministicBlackboardValues* Comp = Cast<UDeterministicBlackboardValues>(Pawn->FindComponentByClass(UDeterministicBlackboardValues::StaticClass()));
	checkf(Comp, TEXT("Pawn must have a UDeterministicBlackboardValues component."));
	Comp->ClientSetBlackboardAILocations(BlackboardValues);
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

	UE_LOG(LogBenchmarkGym, Log, TEXT("Spawning player %d at %s."), PlayerUniqueID, *ChosenSpawnPoint->GetActorLocation().ToString());
	
	if (Player->GetIsSimulated())
	{
		AIControlledPlayers.Emplace(FControllerIntegerPair{ Player, PlayersSpawned });
	}

	PlayersSpawned++;

	return ChosenSpawnPoint;
}