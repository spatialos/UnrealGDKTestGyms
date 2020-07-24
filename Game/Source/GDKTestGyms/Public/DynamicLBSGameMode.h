// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DynamicLBSInfo.h"
#include "DynamicLBSGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API ADynamicLBSGameMode : public AGameModeBase
{
	GENERATED_UCLASS_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(ReplicatedUsing = OnRep_DynamicLBSInfo)
	ADynamicLBSInfo* DynamicLBSInfo;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

private:
	UFUNCTION()
	void OnRep_DynamicLBSInfo();
};
