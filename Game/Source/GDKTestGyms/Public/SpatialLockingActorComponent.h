// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpatialLockingActorComponent.generated.h"

UCLASS(SpatialType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class USpatialLockingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpatialLockingComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "SpatialGDK|Locking")
	int64 AcquireLock();

	UFUNCTION(BlueprintCallable, Category = "SpatialGDK|Locking")
	void ReleaseLock(int64 LockToken);

	UFUNCTION(BlueprintCallable, Category = "SpatialGDK|Locking")
	bool IsLocked() const;

private:
	// Set on the server to be read on the client.
	UPROPERTY(Replicated)
	bool bIsLocked = false;

	int32 LockCounter = 0;
};
