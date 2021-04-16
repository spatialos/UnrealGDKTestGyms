// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerDisconnectController.generated.h"

/**
 * Used for testing that players are cleaned up correctly when they return to the main menu.
 */
UCLASS()
class GDKTESTGYMS_API APlayerDisconnectController : public APlayerController
{
	GENERATED_BODY()

	virtual void SetupInputComponent() override;

	UFUNCTION()
	void MPressed();
	
};
