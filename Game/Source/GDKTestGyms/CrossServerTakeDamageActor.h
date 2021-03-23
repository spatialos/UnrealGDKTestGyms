// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

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
	FVector_NetQuantize RadialDamageOrigin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize DamagePoint;

	UFUNCTION(BlueprintCallable)
	void TestTakeDamage(AController* Controller);
};
