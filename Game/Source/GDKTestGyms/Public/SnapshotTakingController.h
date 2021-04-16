// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SnapshotTakingController.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API ASnapshotTakingController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void TakeSnapshot();

	UFUNCTION(BlueprintNativeEvent)
	void OnSnapshotOutcome(bool bSuccess, const FString& Path);
};
