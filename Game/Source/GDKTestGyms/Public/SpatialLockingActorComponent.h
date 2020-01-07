// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

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
	UPROPERTY(Replicated)
	bool bIsLocked = false;

	int32 LockCounter = 0;
};
