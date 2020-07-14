// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadBalancing/GridBasedLBStrategy.h"
#include "SpatialNetDriver.h"
#include "DynamicGridBasedLBStrategy.generated.h"

UCLASS()
class GDKTESTGYMS_API UDynamicGridBasedLBStrategy : public UGridBasedLBStrategy
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;

	virtual bool ShouldHaveAuthority(const AActor& Actor) override;
	virtual VirtualWorkerId WhoShouldHaveAuthority(const AActor& Actor) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Dynamic Load Balancing")
		TSubclassOf<AActor> ActorClassToMonitor;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"), Category = "Dynamic Load Balancing")
		uint32 MaxActorLoad;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.1"), Category = "Dynamic Load Balancing")
		float BoundaryChangeStep;

private:
	USpatialNetDriver* NetDriver;

	TMap<TWeakObjectPtr<AActor>, FVector2D> ActorPrevPositions;

	void UpdateWorkerBounds(const FVector2D PrevPos, const FVector2D Actor2DLocation, const int32 FromWorkerCellIndex, const int32 ToWorkerCellIndex);
};
