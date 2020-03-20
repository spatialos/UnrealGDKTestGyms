// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrossServerTakeDamageActor.generated.h"

class AController;

UCLASS(Blueprintable)
class ACrossServerTakeDamageActor : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize DamagePoint;

	UFUNCTION(BlueprintCallable)
	void TestTakeDamage(AController* Controller);
};
