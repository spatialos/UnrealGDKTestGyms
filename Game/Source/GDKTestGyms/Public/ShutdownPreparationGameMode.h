// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShutdownPreparationGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API AShutdownPreparationGameMode : public AGameModeBase
{
	GENERATED_BODY()

	virtual void BeginPlay() override;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	UFUNCTION()
	void HandleOnPrepareShutdown();
};
