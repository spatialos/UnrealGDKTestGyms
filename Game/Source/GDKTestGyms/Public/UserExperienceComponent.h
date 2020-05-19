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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceComponent : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceComponent();

	TMap<int32, float> OpenRPCs;
	int32 RequestKey;

public:	
	static constexpr int NumWindowSamples = 100;

	virtual void InitializeComponent() override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server properties
	float ElapsedSeconds;	 

	TArray<float> UpdateRate; // Frequency at which ClientTime is updated
	TArray<float> RoundTripTime; // Client -> Server -> Client
	
	bool bHadClientTimeRep;

	UFUNCTION()
	void OnRep_ClientTime(float DeltaTime);

	void StartRoundtrip();
	void EndRoundtrip(int32 Key); 
	void OnClientOwnershipGained();

	UPROPERTY(replicated, ReplicatedUsing = OnRep_ClientTime)
	float ClientTimeSeconds; // Replicated from server

	UFUNCTION(Server, Reliable)
	void ServerRTT(int32 Key);

	UFUNCTION(Client, Reliable)
	void ClientRTT(int32 Key);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
