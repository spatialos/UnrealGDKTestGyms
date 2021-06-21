// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlackboardValues.h"
#include "DeterministicBlackboardValues.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDeterministicBlackboardValues, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UDeterministicBlackboardValues : public UActorComponent
{
	GENERATED_BODY()
public:

	void InitialApplyBlackboardValues();

	UFUNCTION(BlueprintCallable)
	void ApplyBlackboardValues();

	UFUNCTION(BlueprintCallable)
	void SwapTarget();

	UFUNCTION(Client, Reliable)
	void ClientSetBlackboardAILocations(const FBlackboardValues& InBlackboardValues);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	FTimerHandle TimerHandle;

	UPROPERTY(Replicated)
	bool bBlackboardValuesInitialised = false;

	UPROPERTY(Replicated)
	FBlackboardValues BlackboardValues;
};
