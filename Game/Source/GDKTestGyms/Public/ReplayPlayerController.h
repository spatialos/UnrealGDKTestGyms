// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ReplayPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API AReplayPlayerController : public APlayerController
{
	GENERATED_BODY()

	AReplayPlayerController();

	virtual void SetupInputComponent() override;

	void StartRecordingButtonPressed();
	void StopRecordingButtonPressed();
	void SwitchRecordingModeButtonPressed();
	FString GetRecordingFilename();

	UFUNCTION(Server, Reliable)
	void StartRecordingRPC();

	UFUNCTION(Server, Reliable)
	void StopRecordingRPC();

	int ReplayNumber = 0;
	// Whether to record locally (on the calling client) or on the server authoritative over the playerController
	bool bRecordLocally = false;
};
