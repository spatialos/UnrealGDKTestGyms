// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ControllerIntegerPair.generated.h"

/**
 *
 */
USTRUCT()
struct FControllerIntegerPair
{
	GENERATED_BODY()

	UPROPERTY()
	AController* Controller;

	int Index{ -1 };
	int FrameTimeout{ 0 };
};
