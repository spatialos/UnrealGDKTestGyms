// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceComponent, Log, All);

// User experience metric
//
// This component is designed to track a user experience in a deployment. 
// It tracks 2 key metrics, these are round trip latency and world
// updates. The player controlled component will fire a server RPC 
// every second and track the time it was sent, the server handles
// this RPC and sends back a key which identifies the start time. 
//
// A second metric is world view lateness, this looks at all the 
// other UserExperienceComponent objects in view and tracks how
// often these are updated. This gives us a definition of the 
// world update rate, this is used to average the time 
// between updates. 
//
// Both of these values are averaged by the server and reported. 
// See BenchmarkGymGameMode::ServerUpdateNFRTestMetrics

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceComponent : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceComponent();

	static constexpr int NumWindowSamples = 100;
	TMap<int32, float> OpenRPCs;
	int32 RequestKey;

	struct ObservedUpdate
	{
		float Value{ 0.0f };
		float TimeSinceChange{ 0.0f };
		TArray<float> TrackedChanges;
	};

public:	
	virtual void InitializeComponent() override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server properties
	float ElapsedTime;
	float ServerClientRTT;
	float ServerViewLateness;
	 
	float ClientRTTTimer;

	TMap<TWeakObjectPtr<UUserExperienceComponent>, ObservedUpdate> ObservedComponents; // World observations
	TArray<float> RoundTripTime; // Client -> Server -> Client
	
	void UpdateClientObservations(float DeltaTime);
	float CalculateWorldFrequency();

	void StartRoundtrip();
	void EndRoundtrip(int32 Key); 
	
	UPROPERTY(replicated)
	float ClientTime; // Replicated from server

	UFUNCTION(Server, Reliable)
	void ServerRTT(int32 Key);

	UFUNCTION(Client, Reliable)
	void ClientRTT(int32 Key);

	UFUNCTION(Server, Reliable)
	void ServerReportMetrics(float RTT, float ViewLateness);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
