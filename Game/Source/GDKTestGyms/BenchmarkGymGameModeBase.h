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
	const FString& GetTotalPlayerCommandLineKey() const { return TotalPlayerCommandLineKey; }

	UFUNCTION(BlueprintCallable)
	const FString& GetTotalNPCsCommandLineKey() const { return TotalNPCsCommandLineKey; }

protected:

	// Total number of players that will connect. Used to determine number of clusters and spawn points to create.
	int32 ExpectedPlayers;
	// NPCs will be spread out evenly over the created player clusters.
	int32 TotalNPCs;

	virtual void Tick(float DeltaSeconds) override;
	virtual void ParsePassedValues();

private:

	static const FString TotalPlayerWorkerFlag;
	static const FString TotalNPCsWorkerFlag;
	static const FString TotalPlayerCommandLineKey;
	static const FString TotalNPCsCommandLineKey;

	// Test scenarios
	float SecondsTillPlayerCheck;
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

	virtual void BeginPlay() override;

	void TickPlayersConnectedCheck(float DeltaSeconds);
	void TickFPSCheck(float DeltaSeconds);
	void TickUXMetricCheck(float DeltaSeconds);

	double GetClientRTT() const { return AveragedClientRTTSeconds; }
	double GetClientViewLateness() const { return AveragedClientViewLatenessSeconds; }
	double GetPlayersConnected() const { return ActivePlayers; }
};
