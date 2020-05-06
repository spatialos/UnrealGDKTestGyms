// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceComponent, Log, All);

struct ObservedUpdate
{
	float Value;
	float TimeSinceChange;
	TArray<float> TimeBetweenChanges;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceComponent : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceComponent();

	static constexpr int NumWindowSamples = 100;
public:	
	virtual void InitializeComponent() override;

	void UpdateServerCondition();
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float ServerTime;
	float ClientReportedUpdateRate;

	float ClientTimeSinceServerUpdate;
	
	bool bServerCondition;

	TArray<float> ClientUpdateFrequency;			// Server -> Client frequency
	TArray<float> RoundTripTime;					// Client -> Server -> Client
	
	float CalculateAverage(const TArray<float>& Array);

	TMap<UUserExperienceComponent*, ObservedUpdate> ObservedComponents; // 
	//TMap<UUserExperienceComponent*, ReportedClientMetrics> ClientMetrics;

	FTimerHandle ClientRPCTimer;

	void SendClientRPC();

	UPROPERTY()
	float ClientTime; // Replicated from server

	UFUNCTION(Client, Reliable)
	void ClientUpdateRPC(float TimeOnServer);

	UFUNCTION(Server, Reliable)
	void ServerUpdateResponse(float TimeOnServer);

	UFUNCTION(Server, Reliable)
	void ServerReportMetrics(float UpdatesPerSecond, float WorldUpdatesPerSecond);
};
