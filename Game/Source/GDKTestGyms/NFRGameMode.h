// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NFRGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API ANFRGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void StartPlay() override;
};
