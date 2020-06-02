// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UserExperienceReporter.h"

#include "BenchmarkGymGameModeBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkGymGameModeBase, Log, All);

UCLASS()
class GDKTESTGYMS_API ABenchmarkGymGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABenchmarkGymGameModeBase();

	UFUNCTION(BlueprintCallable)
	const FString& GetTotalPlayerWorkerFlag() const { return TotalPlayerWorkerFlag; }

	UFUNCTION(BlueprintCallable)
	const FString& GetTotalNPCsWorkerFlag() const { return TotalNPCsWorkerFlag; }

	UFUNCTION(BlueprintCallable)
	const FString& GetTotalPlayerCommandLineArg() const { return TotalPlayerCommandLineArg; }

	UFUNCTION(BlueprintCallable)
	const FString& GetTotalNPCsCommandLineArg() const { return TotalNPCsCommandLineArg; }

protected:

	static const FString TotalPlayerWorkerFlag;
	static const FString TotalNPCsWorkerFlag;
	static const FString TotalPlayerCommandLineArg;
	static const FString TotalNPCsCommandLineArg;

	// Total number of players that will connect. Used to determine number of clusters and spawn points to create.
	int32 ExpectedPlayers;
	// NPCs will be spread out evenly over the created player clusters.
	int32 TotalNPCs;
	float SecondsTillPlayerCheck;

	// Test scenarios
	float PrintUXMetric;
	double AveragedClientRTTSeconds; // The stored average of all the client RTTs
	double AveragedClientViewLatenessSeconds; // The stored average of the client view lateness.
	int32 MaxClientRoundTripSeconds; // Maximum allowed roundtrip
	int32 MaxClientViewLatenessSeconds;
	bool bPlayersHaveJoined;
	bool bHasUxFailed;
	bool bHasFpsFailed;
	float MinAcceptableFPS;
	float MinDelayFPS;
	int32 ActivePlayers; // A count of visible UX components

	double GetClientRTT() const { return AveragedClientRTTSeconds; }
	double GetClientViewLateness() const { return AveragedClientViewLatenessSeconds; }
	double GetPlayersConnected() const { return ActivePlayers; }

	virtual void ParsePassedValues();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:

	void ServerUpdateNFRTestMetrics(float DeltaTime);
};
