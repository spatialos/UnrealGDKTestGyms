// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DynamicLBSInfo.h"
#include "DelegateCombinations.h"
#include "DynamicLBSGameMode.generated.h"

DECLARE_DELEGATE_OneParam(DynamicLBSInfoDelegate, ADynamicLBSInfo*);

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

	DynamicLBSInfoDelegate OnDynamicLBSInfoCreated;

	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_DynamicLBSInfo();
};
