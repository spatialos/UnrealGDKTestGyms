// Fill out your copyright notice in the Description page of Project Settings.


#include "NFRGameMode.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

void ANFRGameMode::StartPlay()
{
	if (GetWorld()->IsServer())
	{
		// Server only. This is a because of 4.24 bug where client FPS cannot be set with nullrhi.
		// Clients are set to 60fps by DefaultEngine settings bUseFixedFrameRate and FixedFrameRate.
		// Hopefully both of these will future-proof us against this breaking, only FixedFrameRate is used in 4.24 but in 4.23 MaxFPS is used.
		GEngine->SetMaxFPS(30.0f); 
		GEngine->FixedFrameRate = 30.0f;
	}
	Super::StartPlay();
}
