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

protected:

	static FString ReadFromCommandLineKey;

	// Total number of players that will connect.
	int32 ExpectedPlayers;

	// Replicated so that offloading and zoning servers can get updates.
	UPROPERTY(ReplicatedUsing = OnRepTotalNPCs, BlueprintReadWrite)
	int32 TotalNPCs;

	virtual void ParsePassedValues();

	UFUNCTION(BlueprintNativeEvent)
	void OnRepTotalNPCs();
	void OnRepTotalNPCs_Implementation() {}

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

private:

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
