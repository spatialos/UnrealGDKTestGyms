// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

// Sets default values for this component's properties
UUserExperienceComponent::UUserExperienceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	FTimerHandle Handle;
	GetWorld()->TimerManager->SetTimer(Handle, [this]() -> void
		{
			StartPing(); // Kick off client RPC
		},
		10.0f, // Every 10 seconds
		true); // loop
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld()->GetNetDriver()->IsServer())
	{
		// Update tick
		LastTimeInTicks = FDateTime::Now().GetTicks();
	}
	else
	{
		// Average all viewable ticks 
		for (TObjectIterator<UUserExperienceComponent> it; it; ++it)
		{

		}
	}
}