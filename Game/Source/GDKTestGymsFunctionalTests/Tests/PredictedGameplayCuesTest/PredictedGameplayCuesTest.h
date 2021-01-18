// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CuesGASTestPawn.h"
#include "SpatialFunctionalTest.h"
#include "PredictedGameplayCuesTest.generated.h"

UCLASS()
class APredictedGameplayCuesTest : public ASpatialFunctionalTest
{
	GENERATED_BODY()

	APredictedGameplayCuesTest();

	virtual void PrepareTest() override;

	UPROPERTY(EditInstanceOnly, Category = "Test Settings")
	float DuplicateActivationCheckWaitTime;

private:
	ACuesGASTestPawn* TestPawn;
	APawn* PrevPawn;
	float StepTimer;
	bool bTimerStarted;
};
