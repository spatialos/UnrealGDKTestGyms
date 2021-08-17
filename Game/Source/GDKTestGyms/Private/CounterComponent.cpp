// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CounterComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogCounterComponent);

UCounterComponent::UCounterComponent()
	: Timer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCounterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Timer -= DeltaTime;
	if (Timer <= 0.0f)
	{
		Timer = ReportFrequency;
		UpdateCachedActorCounts();
	}
}

int32 UCounterComponent::GetActorClassCount(TSubclassOf<AActor> ActorClass) const
{
	const int32* CachedCount = CachedClassCounts.Find(ActorClass);
	if (CachedCount == nullptr)
	{
		UE_LOG(LogCounterComponent, Warning, TEXT("Could not get actor class count for class %s"), *ActorClass->GetName());
		return -1;
	}

	return *CachedCount;
}

void UCounterComponent::UpdateCachedActorCounts()
{
	const UWorld* World = GetWorld();
	for (TSubclassOf<AActor> ActorClass : ClassesToCount)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ActorClass, Actors);
		int32& CachedCount = CachedClassCounts.FindOrAdd(ActorClass);
		CachedCount = Actors.Num();

		UE_LOG(LogCounterComponent, Log, TEXT("%s : %d"), *ActorClass->GetName(), CachedCount);
	}
}