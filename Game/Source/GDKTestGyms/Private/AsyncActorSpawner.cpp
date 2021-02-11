// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "AsyncActorSpawner.h"

AAsyncActorSpawner::AAsyncActorSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AAsyncActorSpawner::BeginPlay()
{
	Super::BeginPlay();

	FSoftClassPath SoftClassPath(TEXT("/Game/AsyncPackageLoadingTest/AsyncActor.AsyncActor_C"));
	UClass* Class = SoftClassPath.ResolveClass();

	if (HasAuthority())
	{
		if (Class == nullptr)
		{
			Class = SoftClassPath.TryLoadClass<UObject>();
		}

		UE_LOG(LogTemp, Log, TEXT("Async Test: Auth server spawning async actor"));

		GetWorld()->SpawnActor(Class);
	}
	else
	{
		if (Class == nullptr)
		{
			UE_LOG(LogTemp, Log, TEXT("Async Test: Worker does not have class loaded, test valid for this worker"));
			bClientTestCorrectSetup = true;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Async Test: Worker has class loaded, test not valid for this worker"));
		}
	}
}

