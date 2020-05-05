// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

#include "SpatialNetDriver.h"
#include "Utils/SpatialMetrics.h"

#include "NFRTestConfiguration.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

// Sets default values for this component's properties
UUserExperienceComponent::UUserExperienceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UUserExperienceComponent::UpdateServerCondition()
{
	const UNFRTestConfiguration* Configuration = GetDefault<UNFRTestConfiguration>();

	// Calculate roundtrip average
	if (RoundTripTime.Num() == NumWindowSamples)
	{
		float RoundTrip = CalculateAverage(RoundTripTime) * 1000.0f; // In ms
		if (RoundTrip > Configuration->MaxRoundTrip)
		{
			bServerCondition = false;

			FString Msg = FString::Printf(TEXT("Average roundtrip too large. %.8f / %.8f"), RoundTrip, static_cast<float>(Configuration->MaxRoundTrip));
			UE_LOG(LogUserExperienceComponent, Log, TEXT("%s"), *Msg);
		}
	}

	// Client update frequency
	if (ClientReportedUpdateRate < Configuration->MinClientUpdates)
	{
		FString Msg = FString::Printf(TEXT("Client update frequency too low. %.8f / %.8f"), ClientReportedUpdateRate, static_cast<float>(Configuration->MinClientUpdates));
		UE_LOG(LogUserExperienceComponent, Log, TEXT("%s"), *Msg);
		bServerCondition = false;
	}
}

float UUserExperienceComponent::CalculateAverage(const TArray<float>& Array)
{
	TArray<float> Sorted = Array;
	Sorted.Sort();
	int ElementsToAverage = Sorted.Num()*0.8f; // Exclude 20% of samples

	float Avg = 0.0f;
	for(int i = 0; i < ElementsToAverage; i++)
	{
		Avg += Sorted[i];
	}
	Avg /= ElementsToAverage;
	return Avg;
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld()->IsServer())
	{
		ServerTime += DeltaTime;
		ClientUpdateRPC(ServerTime); 
		
		bool bUXCondition = true;
	
		UpdateServerCondition();

		// Update replicated time to clients
		ClientTime = ServerTime;
	}
	else if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ClientTimeSinceServerUpdate += DeltaTime;

		if (ClientUpdateFrequency.Num() == NumWindowSamples)
		{
			float AverageUpdateFrequency = 1.0f / CalculateAverage(ClientUpdateFrequency);
			ServerReportMetrics(AverageUpdateFrequency, 0.0f);
		}

		for (auto& Update : ObservedComponents)
		{
			Update.Value.TimeSinceChange += DeltaTime;
		}

		// Look at other players around me
#if 0
		for (TObjectIterator<UUserExperienceComponent> It; It; ++It)
		{
			if (*It != this)
			{
				ObservedUpdate& Update = ObservedComponents.[*It];
				
				Update.Value = It->ClientTime;
				Update.TimeBetweenChanges.Push(Update.TimeSinceChange);
				if (Update.TimeBetweenChanges.Num() > NumWindowSamples)
				{
					Update.TimeBetweenChanges.RemoveAt(0);
				}
				Update.TimeSinceChange = 0.0f;
			}
		}
#endif
	}
}

void UUserExperienceComponent::ServerReportMetrics_Implementation(float UpdatesPerSecond, float WorldUpdatesPerSecond)
{
	ClientReportedUpdateRate = UpdatesPerSecond;
}

UFUNCTION(Server, Reliable)
void UUserExperienceComponent::ServerUpdateResponse_Implementation(float Time)
{
	float Diff = ServerTime - Time;
	RoundTripTime.Push(Diff);
	if (RoundTripTime.Num() > NumWindowSamples)
	{
		RoundTripTime.RemoveAt(0);
	}
}

void UUserExperienceComponent::ClientUpdateRPC_Implementation(float Time)
{
	ServerUpdateResponse(Time); // For round-trip

	ClientUpdateFrequency.Push(ClientTimeSinceServerUpdate);
	if (ClientUpdateFrequency.Num() > NumWindowSamples)
	{
		ClientUpdateFrequency.RemoveAt(0);
	}
	ClientTimeSinceServerUpdate = 0.0f;
}