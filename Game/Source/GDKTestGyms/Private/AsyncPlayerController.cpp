// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncPlayerController.h"

#include "EngineUtils.h"

#include "SpatialGDKSettings.h"

#include "AsyncActorSpawner.h"

AAsyncPlayerController::AAsyncPlayerController()
{
}

void AAsyncPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->GetNetMode() == NM_Client)
	{
		GetWorld()->GetTimerManager().SetTimer(TestCheckTimer, this, &AAsyncPlayerController::CheckTestPassed, 1.0f, false);
	}
}

void AAsyncPlayerController::CheckTestPassed()
{
	// Check for three statuses on client before reporting successful test;
		// 1. AsyncActorSpawner indicates that test setup was correct
		// 2. SpatialGDKSettings::bAsyncLoadNewClassesOnEntityCheckout is true
		// 3. An instance of AsyncActor exists in the world
	bool bAsyncTestCorrectlySetup = false;
	bool bAsyncLoad = GetDefault<USpatialGDKSettings>()->bAsyncLoadNewClassesOnEntityCheckout;
	bool bAsyncActorExists = false;

	for (TActorIterator<AAsyncActorSpawner> It(GetWorld()); It; ++It)
	{
		AAsyncActorSpawner* AsyncActorSpawner = *It;
		bAsyncTestCorrectlySetup = AsyncActorSpawner->bClientTestCorrectSetup;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->GetName().Contains(TEXT("AsyncActor_C")))
		{
			bAsyncActorExists = true;
			break;
		}
	}

	if (bAsyncTestCorrectlySetup && bAsyncLoad)
	{
		if (bAsyncActorExists)
		{
			UE_LOG(LogTemp, Log, TEXT("Async Test: Test passed."));
			UpdateTestPassed(true);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Async Test: Test valid but actor not spawned yet, checking again in 1 second."));
			GetWorld()->GetTimerManager().SetTimer(TestCheckTimer, this, &AAsyncPlayerController::CheckTestPassed, 1.0f, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Async Test: Test invalid on this client - client correctly setup %d, async load on: %d"), bAsyncTestCorrectlySetup, bAsyncLoad);
	}
}

void AAsyncPlayerController::UpdateTestPassed_Implementation(bool bInTestPassed)
{
	bTestPassed = bInTestPassed;
}
