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
		Time += DeltaTime;
		ClientUpdatePrediction(sinf(Time*PredictionSineSpeed)*PredictionSineMagnitude, cosf(Time*PredictionSineSpeed)*PredictionSineMagnitude); // Value and tangent
	}
	else if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		TimeSinceLastUpdate += DeltaTime;
	}
}

void UUserExperienceComponent::ServerFail_Implementation(const FString& ClientError)
{
	UE_LOG(LogUserExperienceComponent, Error, TEXT("%s"), *ClientError);
}

void UUserExperienceComponent::ClientUpdatePrediction_Implementation(float Point, float Velocity)
{
	if (CheckPredictionError(Point, CalculateLocalPrediction()))
	{
		NumCorrections++;
		UE_LOG(LogUserExperienceComponent, Log, TEXT("Received a correction, (%d / %d)"), NumCorrections, MaxRPCFailures);
	}

	if (NumCorrections > MaxRPCFailures)
	{
		FString FailureMessage = FString::Printf(TEXT("Client %s needed too many corrections (%d / %d)"), *GetWorld()->GetNetDriver()->GetName(), NumCorrections, MaxRPCFailures);
		UE_LOG(LogUserExperienceComponent, Error, TEXT("%s. Sending server RPC"), *FailureMessage);
		ServerFail(FailureMessage);
	}

	TimeSinceLastUpdate = 0.0f;
	LastPoint = Point;
	LastVelocity = Velocity;
}