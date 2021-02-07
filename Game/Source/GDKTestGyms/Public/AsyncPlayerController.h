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
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(Server, Reliable)
	void UpdateTestPassed(bool bInTestPassed);

	UPROPERTY(BlueprintReadWrite)
	bool bTestPassed = false;

	bool bOutputTestFailure = true;
};
