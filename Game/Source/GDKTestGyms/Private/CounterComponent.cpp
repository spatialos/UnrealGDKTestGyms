// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CounterComponent.h"

#include "Engine/World.h"
#include "EngineClasses/SpatialPackageMapClient.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Kismet/GameplayStatics.h"
#include "SpatialConstants.h"
#include "SpatialView/EntityView.h"

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

int32 UCounterComponent::GetActorTotalCount(const TSubclassOf<AActor>& ActorClass) const
{
	const FActorCountInfo* CachedCount = CachedClassCounts.Find(ActorClass);
	if (CachedCount == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get actor count for class %s"), *ActorClass->GetName());
		return -1;
	}

	return CachedCount->TotalCount;
}

int32 UCounterComponent::GetActorAuthCount(const TSubclassOf<AActor>& ActorClass) const
{
	const FActorCountInfo* CachedCount = CachedClassCounts.Find(ActorClass);
	if (CachedCount == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get actor class count for class %s"), *ActorClass->GetName());
		return -1;
	}

	return CachedCount->AuthCount;
}

void UCounterComponent::UpdateCachedActorCounts()
{
	const UWorld* World = GetWorld();
	USpatialNetDriver* SpatialDriver = Cast<USpatialNetDriver>(World->GetNetDriver());
	USpatialPackageMapClient* PackageMap = SpatialDriver->PackageMap;
	const SpatialGDK::EntityView& View = SpatialDriver->Connection->GetView();

	UE_LOG(LogTemp, Log, TEXT("Updating cached actor count for %s"), SpatialDriver->Connection->GetWorkerId());

	for (TSubclassOf<AActor> ActorClass : ClassesToCount)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ActorClass, Actors);

		int32 AuthCount = 0;
		for (AActor* Actor : Actors)
		{
			if (Actor->HasAuthority())
			{
				AuthCount++;
			}
			else if (PackageMap != nullptr)
			{
				// During actor authority handover, there's a period where no server will believe it has authority over
				// the Unreal actor, but will still have authority over the entity. To better minimize this period, use
				// the spatial authority as a fallback validation.
				Worker_EntityId EntityId = PackageMap->GetEntityIdFromObject(Actor);
				const SpatialGDK::EntityViewElement* Element = View.Find(EntityId);
				if (Element != nullptr && Element->Authority.Contains(SpatialConstants::SERVER_AUTH_COMPONENT_SET_ID))
				{
					AuthCount++;
				}
			}
		}

		FActorCountInfo& ActorCountInfo = CachedClassCounts.FindOrAdd(ActorClass);
		ActorCountInfo.TotalCount = Actors.Num();
		ActorCountInfo.AuthCount = AuthCount;

		UE_LOG(LogTemp, Log, TEXT("Auth Count: %s - %d"), *ActorClass->GetName(), ActorCountInfo.AuthCount);
	}
}
