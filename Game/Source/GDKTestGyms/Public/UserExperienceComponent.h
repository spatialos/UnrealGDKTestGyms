// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceComponent, Log, All);

struct ObservedUpdate
{
	float Value{ 0.0f };
	float TimeSinceChange{ 0.0f };
	TArray<float> TrackedChanges;
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

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server properties
	float ElapsedTime;
	float ServerClientRTT;
	float ServerViewLateness;
	 
	float ClientRTTTimer;

	void UpdateClientObservations(float DeltaTime);
	TMap<UUserExperienceComponent*, ObservedUpdate> ObservedComponents; // World observations
	TArray<float> RoundTripTime; // Client -> Server -> Client
	
	float CalculateWorldFrequency();

	void SendServerRPC();

	UPROPERTY(replicated)
	float ClientTime; // Replicated from server

	UFUNCTION(Server, Reliable)
	void ServerRTT(float Time);

	UFUNCTION(Client, Reliable)
	void ClientRTT(float Time);

	UFUNCTION(Server, Reliable)
	void ServerReportMetrics(float RTT, float ViewLateness);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
