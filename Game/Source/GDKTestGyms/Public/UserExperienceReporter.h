// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceReporter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceReporter, Log, All);

// User experience metric
//
// This component takes the recorded values from UserExperienceComponent,
// averages them and makes them accessible for the BenchmarkGymGameMode to query.
//
// The BenchmarkGymGameMode will report these metrics to Grafana or use them 
// to fail native test scenarios by printing a log message.

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceReporter : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceReporter();
public:	
	// Valid on server
	float ServerRTT;
	float ServerViewDelta;
	bool bFrameRateValid;

	void InitializeComponent() override;
	 
	void OnClientOwnershipGained() override;
	void ReportMetrics();

	UFUNCTION(Server, reliable)
	void ServerReportedMetrics(float RTTSeconds, float ViewDeltaSeconds, bool bInFrameRateValid);
};
