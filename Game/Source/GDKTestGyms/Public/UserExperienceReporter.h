// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceReporter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceReporter, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceReporter : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceReporter();
public:	
	float ServerRTT;
	float ServerViewLateness;

	void InitializeComponent() override;
	 
	void OnClientOwnershipGained() override;
	void ReportMetrics();
	UFUNCTION(Server, reliable)
	void ServerReportedMetrics(float RTT, float ViewLateness);
};
