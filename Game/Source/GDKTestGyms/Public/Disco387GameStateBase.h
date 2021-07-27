// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Disco387GameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API ADisco387GameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	virtual void PostInitializeComponents() override;
};
