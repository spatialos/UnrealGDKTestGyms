// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClientTravelController.generated.h"

UCLASS()
class AClientTravelController : public APlayerController
{
	GENERATED_BODY()

private:
	UFUNCTION(BlueprintCallable)
	void OnCommandTravel();
};
