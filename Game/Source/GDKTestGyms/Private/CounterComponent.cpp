// Fill out your copyright notice in the Description page of Project Settings.

#include "CounterComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogCounterComponent);

UCounterComponent::UCounterComponent()
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
		CountClasses();
	}
}

int32 UCounterComponent::GetActorClassCount(TSubclassOf<AActor> ActorClass) const
{
	const int32* CachedCount = CachedClassCounts.Find(ActorClass);
	if (CachedCount == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get actor class count for class %s"), *ActorClass->GetDisplayNameText().ToString());
		return -1;
	}

	return *CachedCount;
}

void UCounterComponent::CountClasses()
{
	for (TSubclassOf<AActor> ActorClass : ClassesToCount)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, Actors);
		int32& CachedCount = CachedClassCounts.FindOrAdd(ActorClass);
		CachedCount = Actors.Num();

		UE_LOG(LogCounterComponent, Log, TEXT("%s : %d"), *ActorClass->GetDisplayNameText().ToString(), CachedCount);
	}
}
