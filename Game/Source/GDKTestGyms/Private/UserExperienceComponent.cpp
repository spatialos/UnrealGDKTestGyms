// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

// Sets default values for this component's properties
UUserExperienceComponent::UUserExperienceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld()->IsServer())
	{
		ServerTime += DeltaTime;
		ClientUpdate(ServerTime); 

		if (RoundTripTime.Num() == NumWindowSamples)
		{
			float Avg = 0.0f;
			for (auto& N : RoundTripTime)
			{
				Avg += N;
			}
			Avg /= RoundTripTime.Num();

			FString Msg = FString::Printf(TEXT("Average roundtrip %.8f"), Avg*1000.0f);
			UE_LOG(LogUserExperienceComponent, Log, TEXT("%s"), *Msg);
		}
	}
	else if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ClientTimeSinceServerUpdate += DeltaTime;

		if (ClientUpdateFrequency.Num() == NumWindowSamples)
		{
			float Avg = 0.0f;
			for (auto& N : ClientUpdateFrequency)
			{
				Avg += N;
			}
			Avg /= ClientUpdateFrequency.Num();

			FString Msg = FString::Printf(TEXT("Client updates per second %.8f"), 1.0f / Avg);
			ServerLog(Msg);
		}
	}
}

void UUserExperienceComponent::ServerLog_Implementation(const FString& ClientError)
{
	UE_LOG(LogUserExperienceComponent, Log, TEXT("%s"), *ClientError);
}

UFUNCTION(Server, Reliable)
void UUserExperienceComponent::ServerResponse_Implementation(float Time)
{
	float Diff = ServerTime - Time;
	RoundTripTime.Push(Diff);
	if (RoundTripTime.Num() > NumWindowSamples)
	{
		RoundTripTime.RemoveAt(0);
	}
}

void UUserExperienceComponent::ClientUpdate_Implementation(float Time)
{
	ServerResponse(Time); // For round-trip

	ClientUpdateFrequency.Push(ClientTimeSinceServerUpdate);
	if (ClientUpdateFrequency.Num() > NumWindowSamples)
	{
		ClientUpdateFrequency.RemoveAt(0);
	}
	ClientTimeSinceServerUpdate = 0.0f;
}