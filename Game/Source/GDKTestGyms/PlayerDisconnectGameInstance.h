// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "GDKTestGymsGameInstance.h"

#include "PlayerDisconnectGameInstance.generated.h"

UCLASS()
class GDKTESTGYMS_API UPlayerDisconnectGameInstance : public UGDKTestGymsGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void ReturnToMainMenu() override;
};
