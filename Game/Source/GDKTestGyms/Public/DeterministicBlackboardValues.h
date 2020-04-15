// Fill out your copyright notice in the Description page of Project Settings.

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
	void ApplyValues();

	UFUNCTION(Client, Reliable)
	void ClientSetBlackboardAILocations(const FBlackboardValues& InBlackboardValues);
protected:
	FTimerHandle TimerHandle;
	FBlackboardValues BlackboardValues;
};
