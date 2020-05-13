// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

#include "NFRTestConfiguration.h"
#include "Net/UnrealNetwork.h"
#include "SpatialNetDriver.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

namespace
{
	float Calculate80thPctAverage(const TArray<float>& Array)
	{
		TArray<float> Sorted = Array;
		Sorted.Sort();
		int ElementsToAverage = Sorted.Num()*0.8f; // Exclude 20% of samples

		float Avg = 0.0f;
		for (int i = 0; i < ElementsToAverage; i++)
		{
			Avg += Sorted[i];
		}
		Avg /= ElementsToAverage;
		return Avg;
	}
}

// Sets default values for this component's properties
UUserExperienceComponent::UUserExperienceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UUserExperienceComponent::InitializeComponent()
{
	ElapsedTime = 0.0f;
	ServerClientRTT = 0.0f;
	ServerViewLateness = 0.0f;
	ClientRTTTimer = 0.0f; 
	SetIsReplicated(true);
	UActorComponent::InitializeComponent();
}

void UUserExperienceComponent::SendServerRPC()
{
	ServerRTT(ElapsedTime);
}

void UUserExperienceComponent::UpdateClientObservations(float DeltaTime)
{
	// Prune out any components which went out of view to avoid skewing results
	TMap<UUserExperienceComponent*, ObservedUpdate> NewObservations;
	for (TObjectIterator<UUserExperienceComponent> It; It; ++It)
	{
		if (this != *It && It->GetOwner() && It->GetOwner()->GetWorld() == GetWorld())
		{
			if (ObservedUpdate* PreviousUpdate = ObservedComponents.Find(*It))
			{
				// Carry over into new observations
				NewObservations.Add(*It, MoveTemp(*PreviousUpdate));
			}
			else
			{
				NewObservations.Add(*It, ObservedUpdate{});
			}
		}
	}
	ObservedComponents = NewObservations;

	for (TPair<UUserExperienceComponent*, ObservedUpdate>& ObservationPair : ObservedComponents)
	{
		ObservedUpdate& Observation = ObservationPair.Value;
		UUserExperienceComponent* Component = ObservationPair.Key;

		Observation.TimeSinceChange += DeltaTime;
		if (Observation.Value != Component->ClientTime)
		{
			// New value observed.
			Observation.Value = Component->ClientTime;
			Observation.TrackedChanges.Push(Observation.TimeSinceChange);
			Observation.TimeSinceChange = 0.0f;

			if (Observation.TrackedChanges.Num() > NumWindowSamples)
			{
				Observation.TrackedChanges.RemoveAt(0);
			}
		}
	}
}

float UUserExperienceComponent::CalculateWorldFrequency()
{
	float Frequency = 0.0f;
	float ServerFrameRate = 30.0f; // TODO: Find a good way of getting this properly
	for (TPair<UUserExperienceComponent*, ObservedUpdate>& ObservationPair : ObservedComponents)
	{
		ObservedUpdate& Observation = ObservationPair.Value;
		UUserExperienceComponent* Component = ObservationPair.Key;

		if (!Component->IsActive())
		{
			continue;
		}

		if (Observation.TrackedChanges.Num() == NumWindowSamples)
		{
			float AverageUpdateRate = Calculate80thPctAverage(Observation.TrackedChanges);
			Frequency += AverageUpdateRate;
		}
		else
		{
			Frequency += 0.0f;
		}
	}
	
	Frequency /= static_cast<float>(ObservedComponents.Num());
	return Frequency;
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ClientRTTTimer -= DeltaTime;
		if (ClientRTTTimer < 0.0f)
		{
			SendServerRPC();
			ClientRTTTimer = 1.0f;
		} 
	}

	ElapsedTime += DeltaTime;

	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		// Update replicated time to clients
		ClientTime = ElapsedTime;
	}
	else if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		UpdateClientObservations(DeltaTime);

		// Calculate world view average adjusted by number of observations
		float RTT = Calculate80thPctAverage(RoundTripTime)*1000.0f;
		float WorldUpdates = CalculateWorldFrequency();
		ServerReportMetrics(RTT, WorldUpdates);

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

void UUserExperienceComponent::ServerReportMetrics_Implementation(float RTT, float ViewLateness)
{
	ServerClientRTT = RTT;
	ServerViewLateness = ViewLateness;
}

void UUserExperienceComponent::ClientRTT_Implementation(float Time)
{
	float Diff = ElapsedTime - Time;
	RoundTripTime.Push(Diff);
	if (RoundTripTime.Num() > NumWindowSamples)
	{
		RoundTripTime.RemoveAt(0);
	}
}

void UUserExperienceComponent::ServerRTT_Implementation(float Time)
{
	ClientRTT(Time); // For round-trip
}

void UUserExperienceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUserExperienceComponent, ClientTime);
}