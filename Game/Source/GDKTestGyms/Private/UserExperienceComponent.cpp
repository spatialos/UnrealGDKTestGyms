// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

// Sets default values for this component's properties
UUserExperienceComponent::UUserExperienceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	FVector2D sp = RealMovement(1.0f);
	FVector2D sv = RealVelocity(1.0f);

// 	float t = 0.0f;
// 	for (int i = 0; i < 1000; i++)
// 	{
// 		FVector2D pp = PredirectedMovement(sp, sv, t);
// 		FVector2D ap = RealMovement(1.0f + t);
// 		float d = (pp - ap).SizeSquared();
// 		if (CheckFailedPrediction(ap, pp))
// 		{
// 			int fail = 0;
// 		}
// 		t += 0.001f; // 1ms 
// 	}
}

float UUserExperienceComponent::CheckFailedPrediction(FVector2D RealPoint, FVector2D SimPoint) const
{
	// Error rate = (sin(T{n+1}) - (sin(T{n}) + (T{n+1}-T{n})*cos(T{n}))
// 	float T1 = MinRPCRate;
// 	float T0 = 0.0f;
// 	float RealPositon = PredictionSineMagnitude * sinf(T1*PredictionSineSpeed);
// 	float PredictedPosition = sinf(T0*PredictionSineMagnitude) + PredictionSineMagnitude * (T1-T0)*cosf(T0*PredictionSineMagnitude);
// 	float MaxError = fabs(RealPositon - (PredictedPosition)); // fabs(PredictionSineMagnitude*(sinf(T1) - (sinf(T0) + (T1 - T0)*cosf(T0))));
// 	float ActualError = fabs(PointA - PointB);
// 	return ActualError > MaxError;
	float Dist = (RealPoint - SimPoint).SizeSquared();
	float MaxDist = (RealMovement(MinRPCRate) - (RealMovement(0.0f) + MinRPCRate * RealVelocity(0.0f))).SizeSquared();
	return Dist > MaxDist;
}

FVector2D UUserExperienceComponent::RealMovement(float Time) const
{
	Time *= PredictionSineSpeed;
	return FVector2D(cosf(Time), sinf(Time))*PredictionSineMagnitude;
}

FVector2D UUserExperienceComponent::RealVelocity(float Time) const
{
	//return PredictionSineSpeed * cosf(Time*PredictionSineSpeed)*PredictionSineMagnitude;
	//return (RealMovement(Time + 0.01f) - RealMovement(Time - 0.01f)) / 0.02f;
	Time *= PredictionSineSpeed;
	return FVector2D(-sinf(Time), cosf(Time))*PredictionSineSpeed*PredictionSineMagnitude;
}

FVector2D UUserExperienceComponent::PredirectedMovement(FVector2D Last, FVector2D Velocity, float Time) const
{
	return Last + Velocity * Time;
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld()->IsServer())
	{
		ServerTime += DeltaTime;
		FVector2D NewValue = RealMovement(ServerTime);
		FVector2D NewVelocity = RealVelocity(ServerTime);
		ClientUpdatePrediction(NewValue, NewVelocity); // Value and tangent
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

void UUserExperienceComponent::ClientUpdatePrediction_Implementation(FVector2D Point, FVector2D Velocity)
{
	FVector2D Prediction = PredirectedMovement(LastPoint, LastVelocity, TimeSinceLastUpdate);
	if (CheckFailedPrediction(Point, Prediction))
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