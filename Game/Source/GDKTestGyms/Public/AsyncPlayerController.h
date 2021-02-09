// Fill out your copyright notice in the Description page of Project Settings.

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
