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
	static constexpr float PredictionSineMagnitude = 1.0f;
	static constexpr float PredictionSineSpeed = 0.001f * 3.14159f * 2.0f; // 100 seconds for full cycle
	static constexpr float MinRPCRate = 0.5f; // RPC every half a second

	static constexpr int MaxRPCFailures = 10;

	float CheckFailedPrediction(FVector2D PointA, FVector2D PointB) const;

	FVector2D RealMovement(float Time) const;
	FVector2D RealVelocity(float Time) const;
	FVector2D PredirectedMovement(FVector2D Last, FVector2D Velocity, float Time) const;
public:	
	virtual void InitializeComponent()
	{
		ServerTime = 0.0f;
		LastPoint = LastVelocity = FVector2D::ZeroVector;
		TimeSinceLastUpdate = 0.0f;
	}
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float ServerTime;
	float TimeSinceLastUpdate;

	FVector2D LastPoint;
	FVector2D LastVelocity;

	int NumCorrections = 0;
	
	UFUNCTION(Client, Reliable)
	void ClientUpdatePrediction(FVector2D Point, FVector2D Velocity);

	UFUNCTION(Server, Reliable)
	void ServerFail(const FString& Message);
};
