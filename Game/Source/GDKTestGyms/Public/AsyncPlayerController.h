// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AsyncPlayerController.generated.h"

/**
 *
 */
UCLASS()
class GDKTESTGYMS_API AAsyncPlayerController : public APlayerController
{
	GENERATED_BODY()

		AAsyncPlayerController();
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
		void CheckTestPassed();

	UFUNCTION(Server, Reliable)
		void UpdateTestPassed(bool bInTestPassed);

	UPROPERTY(BlueprintReadWrite)
		bool bTestPassed = false;

	FTimerHandle TestCheckTimer;
};
