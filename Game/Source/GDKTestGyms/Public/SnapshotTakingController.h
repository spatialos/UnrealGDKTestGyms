// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SnapshotTakingController.generated.h"

/**
 * A controller with a keybinding to take snapshots at runtime (only works in-editor)
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
