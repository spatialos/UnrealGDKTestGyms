// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "BenchmarkGymPlayerState.generated.h"

UCLASS()
class GDKTESTGYMS_API ABenchmarkGymPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual bool ShouldBroadCastWelcomeMessage(bool bExiting = false) override;

};

