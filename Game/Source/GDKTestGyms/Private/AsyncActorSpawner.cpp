// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncActorSpawner.h"

AAsyncActorSpawner::AAsyncActorSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AAsyncActorSpawner::BeginPlay()
{
	Super::BeginPlay();

	FSoftClassPath SoftClassPath(TEXT("/Game/Actors/AsyncActor.AsyncActor_C"));
	UClass* Class = SoftClassPath.ResolveClass();

	if (HasAuthority())
	{
		if (Class == nullptr)
		{
			Class = SoftClassPath.TryLoadClass<UObject>();
		}

		UE_LOG(LogTemp, Display, TEXT("Async Test: Auth server spawning async actor"));

		GetWorld()->SpawnActor(Class);
	}
	else
	{
		if (Class == nullptr)
		{
			UE_LOG(LogTemp, Display, TEXT("Async Test: Worker does not have class loaded, test valid"));
		}
	}
}

