// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "BlackboardValues.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UserExperienceComponent.h"

#include "BenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGym, Log, All);

typedef TPair<TWeakObjectPtr<AController>, int> ControllerIntegerPair;

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameMode();

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
private:
	TArray<FBlackboardValues> PlayerRunPoints;
	TArray<FBlackboardValues> NPCRunPoints;
	void GenerateTestScenarioLocations();

	void BeginPlay() override; 
	double AggregatedClientRTT; // Aggregated client RTT
	double AggregatedClientViewLateness; // Aggregated client view latness
	double GetClientRTT() const { return AggregatedClientRTT; }
	double GetClientViewLateness() const { return AggregatedClientViewLateness; }
	void ServerUpdateNFRTestMetrics();

	
	TArray<ControllerIntegerPair> AIControlledPlayers;
	TArray<TWeakObjectPtr<UUserExperienceComponent>> PlayerUXComponents;

	bool bHasUpdatedMaxActorsToReplicate;
	// Custom density spawning parameters.
	bool bInitializedCustomSpawnParameters;
	// Total number of players that will connect. Used to determine number of clusters and spawn points to create.
	int32 ExpectedPlayers;
	// Number of players per cluster. Players only see other players in the same cluster.
	// Number of generated clusters is Ceil(TotalPlayers / PlayerDensity)
	int32 PlayerDensity;
	// NPCs will be spread out evenly over the created player clusters.
	int32 TotalNPCs;
	int32 NumPlayerClusters;
	int32 PlayersSpawned;
	TArray<AActor*> SpawnPoints;
	TSubclassOf<APawn> NPCPawnClass;
	TMap<int32, AActor*> PlayerIdToSpawnPointMap;
	int32 NPCSToSpawn;
	float SecondsTillPlayerCheck;
	void Tick(float DeltaSeconds) override;
	bool ShouldUseCustomSpawning();
	void CheckCmdLineParameters();
	void ParsePassedValues();
	void ClearExistingSpawnPoints();
	void SpawnNPCs(int NumNPCs);
	void SpawnNPC(const FVector& SpawnLocation, const FBlackboardValues& BlackboardValues);
	// Generates a grid of points centered at (0, 0), as square-like as possible. A row has a fixed y-value, and a column a fixed x-value.
	static void GenerateGridSettings(int DistBetweenPoints, int NumPoints, int& NumRows, int& NumCols, int& MinRelativeX, int& MinRelativeY);
	void GenerateSpawnPointClusters(int NumClusters);
	void GenerateSpawnPoints(int CenterX, int CenterY, int SpawnPointsNum);
	void Logout(AController* Controller) override;
};
