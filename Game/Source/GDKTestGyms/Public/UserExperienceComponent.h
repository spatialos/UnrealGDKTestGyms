// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceComponent, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceComponent : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceComponent();

	// Prediction sinusoid 
	static constexpr float PredictionSineMagnitude = 100.0f;
	static constexpr float PredictionSineSpeed = 20.0f; 
	static constexpr float MinRPCRate = 0.5f;

	static constexpr int MaxRPCFailures = 10;

	float CheckPredictionError(float PointA, float PointB) const
	{
		// Error rate = (sin(T{n+1}) - (sin(T{n}) + (T{n+1}-T{n})*cos(T{n}))
		float InvRPCRate = 1.0f / MinRPCRate; // Min RPC time in seconds
		float T1 = InvRPCRate;
		float T0 = 0.0f;
		float MaxError = PredictionSineMagnitude*(sinf(T1) - (sinf(T0) + (T1 - T0)*cosf(T0)));
		float ActualError = fabs(PointA - PointB);
		return ActualError < MaxError;
	}
public:	
	virtual void InitializeComponent()
	{
		Time = 0.0f;
		LastPoint = LastVelocity = 0.0f;
		TimeSinceLastUpdate = 0.0f;
	}
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float Time;
	float TimeSinceLastUpdate;

	float LastPoint;
	float LastVelocity;

	int NumCorrections = 0;

	// Predict based on last + velocity * timeSinceUpdate
	float CalculateLocalPrediction() const { return LastPoint + LastVelocity * TimeSinceLastUpdate; }
	
	UFUNCTION(Client, Reliable)
	void ClientUpdatePrediction(float Point, float Velocity);

	UFUNCTION(Server, Reliable)
	void ServerFail(const FString& Message);
};
