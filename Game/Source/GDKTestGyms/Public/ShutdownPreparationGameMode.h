// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShutdownPreparationGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShutdownPreparationGameMode, Log, All);

/**
 * Game mode that listens for the shutdown preparation event and sends all players to the main menu when it is triggered.
 * It also rejects any new player logins if shutdown preparation has been started.
 */
UCLASS()
class GDKTESTGYMS_API AShutdownPreparationGameMode : public AGameModeBase
{
	GENERATED_BODY()

	AShutdownPreparationGameMode();

	virtual void BeginPlay() override;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	UFUNCTION()
	void HandleOnPrepareShutdown();
};
