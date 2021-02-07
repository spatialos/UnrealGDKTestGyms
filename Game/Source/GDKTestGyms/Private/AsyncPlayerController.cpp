// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncPlayerController.h"

#include "EngineUtils.h"

#include "SpatialGDKSettings.h"

#include "AsyncActorSpawner.h"

AAsyncPlayerController::AAsyncPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AAsyncPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AAsyncPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetWorld()->GetNetMode() == NM_Client)
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

		if (bAsyncTestCorrectlySetup && bAsyncLoad && bAsyncActorExists)
		{
			UpdateTestPassed(true);
		}
		else if (bOutputTestFailure)
		{
			UE_LOG(LogTemp, Warning, TEXT("Test invalid on this client - client correctly setup %d, async load on: %d, async actor exists: %d"), bAsyncTestCorrectlySetup, bAsyncLoad, bAsyncActorExists);
			bOutputTestFailure = false;
		}
	}
}

void AAsyncPlayerController::UpdateTestPassed_Implementation(bool bInTestPassed)
{
	bTestPassed = bInTestPassed;
}
