// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadBalancing/GridBasedLBStrategy.h"
#include "DynamicGridBasedLBStrategy.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDynamicGridBasedLBStrategy, Log, All)

/**
 * 
 */
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

	void IncreseActorCounter();
	void DecreaseActorCounter();

private:
	uint32 ActorCounter;
	TMap<TWeakObjectPtr<AActor>, FVector2D> ActorPrevPositions;
};
