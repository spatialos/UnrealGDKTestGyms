// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

#include "Net/UnrealNetwork.h"
#include "SpatialNetDriver.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

namespace
{
	float Calculate80thPctAverage(TArray<float> Array) // In ascending order 
	{
		Array.Sort();
		int ElementsToAverage = Array.Num()*0.8f; // Exclude 20% of samples

		float Avg = 0.0f;
		for (int i = 0; i < ElementsToAverage; i++)
		{
			Avg += Array[i];
		}
		Avg /= static_cast<float>(ElementsToAverage) + 0.00001f;
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
	UActorComponent::InitializeComponent();
	ElapsedTime = 0.0f;
	ServerClientRTT = 0.0f;
	ServerViewLateness = 0.0f;
	ClientRTTTimer = 0.0f; 
	RequestKey = 0;
	SetIsReplicated(true);
}

void UUserExperienceComponent::StartRoundtrip()
{
	if (OpenRPCs.Contains(RequestKey))
	{
		EndRoundtrip(RequestKey);
	}
	int32 NewRPC = RequestKey++;
	OpenRPCs.Add(NewRPC, ElapsedTime);
	ServerRTT(NewRPC);
}

void UUserExperienceComponent::EndRoundtrip(int32 Key)
{
	if (float* StartTime = OpenRPCs.Find(Key))
	{
		float Diff = ElapsedTime - *StartTime;
		RoundTripTime.Push(Diff);
		if (RoundTripTime.Num() > NumWindowSamples)
		{
			RoundTripTime.RemoveAt(0);
		}
	}
}

void UUserExperienceComponent::UpdateClientObservations(float DeltaTime)
{
	// Prune out any components which went out of view to avoid skewing results
	TMap<TWeakObjectPtr<UUserExperienceComponent>, ObservedUpdate> NewObservations;
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

	for (TPair<TWeakObjectPtr<UUserExperienceComponent>, ObservedUpdate>& ObservationPair : ObservedComponents)
	{
		ObservedUpdate& Observation = ObservationPair.Value;
		UUserExperienceComponent* Component = ObservationPair.Key.Get(); // Valid, it was just pulled found TObjectIterator

		if (Component == nullptr)
		{
			continue;
		}

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
	for (TPair<TWeakObjectPtr<UUserExperienceComponent>, ObservedUpdate>& ObservationPair : ObservedComponents)
	{
		ObservedUpdate& Observation = ObservationPair.Value;
		UUserExperienceComponent* Component = ObservationPair.Key.Get();

		if (Component == nullptr || !Component->IsActive())
		{
			continue;
		}

		if (Observation.TrackedChanges.Num() == NumWindowSamples)
		{
			float AverageUpdateRate = Calculate80thPctAverage(Observation.TrackedChanges);
			Frequency += AverageUpdateRate;
		}
	}
	
	Frequency /= static_cast<float>(ObservedComponents.Num()) + 0.00001f;
	return Frequency;
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	AActor* OwnerActor = GetOwner();
	if (OwnerActor->HasLocalNetOwner())
	{
		ClientRTTTimer -= DeltaTime;
		if (ClientRTTTimer < 0.0f)
		{
			StartRoundtrip();
			ClientRTTTimer = 1.0f;
		} 
	}

	ElapsedTime += DeltaTime;

	if (OwnerActor->HasAuthority())
	{
		// Update replicated time to clients
		ClientTime = ElapsedTime;
		OwnerActor->ForceNetUpdate();
	}
	else if (OwnerActor->HasLocalNetOwner())
	{
		UpdateClientObservations(DeltaTime);

		// Calculate world view average adjusted by number of observations
		float RTT = Calculate80thPctAverage(RoundTripTime)*1000.0f;
		float WorldUpdates = CalculateWorldFrequency();
		ServerReportMetrics(RTT, WorldUpdates);
	}
}

void UUserExperienceComponent::ServerReportMetrics_Implementation(float RTT, float ViewLateness)
{
	ServerClientRTT = RTT;
	ServerViewLateness = ViewLateness;
}

void UUserExperienceComponent::ServerRTT_Implementation(int32 Key)
{
	ClientRTT(Key); // For round-trip
}

void UUserExperienceComponent::ClientRTT_Implementation(int32 Key)
{
	EndRoundtrip(Key);
}

void UUserExperienceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUserExperienceComponent, ClientTime);
}