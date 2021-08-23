#include "ProtoImposterSystem.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "LoadBalancing/LegacyLoadBalancingCommon.h"

#include "ScavengersHubGameFramework/ScavengersHubIsmActor.h"
//#include "ScavengersHubIsmActor.h"

UProtoImposterSystem::UProtoImposterSystem() {}

TArray<SpatialGDK::FLBDataStorage*> UProtoImposterSystem::GetData()
{
	TArray<SpatialGDK::FLBDataStorage*> Storages;
	Storages.Add(&Positions);
	Storages.Add(&ActorMetadata);

	return Storages;
}

void UProtoImposterSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	USpatialGameInstance* GameInstance = Cast<USpatialGameInstance>(GetOuter());
	if (!ensure(GameInstance != nullptr))
	{
		return;
	}

	WorldChangedDelegate = GameInstance->OnWorldChanged().AddUObject(this, &UProtoImposterSystem::OnWorldChanged);
	OnWorldChanged(nullptr, GameInstance->GetWorld());
}

void UProtoImposterSystem::Deinitialize()
{
	USpatialGameInstance* GameInstance = Cast<USpatialGameInstance>(GetOuter());
	if (!ensure(GameInstance != nullptr))
	{
		return;
	}

	GameInstance->OnWorldChanged().Remove(WorldChangedDelegate);

	Super::Deinitialize();
}

void UProtoImposterSystem::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	if (OldWorld != nullptr)
	{
		OldWorld->OnPostTickDispatch().Remove(PostTickDispatchDelegate);
		if (WorldActor)
		{
			WorldActor->Destroy();
			WorldActor = nullptr;
		}
	}

	if (NewWorld != nullptr)
	{
		PostTickDispatchDelegate = NewWorld->OnPostTickDispatch().AddUObject(this, &UProtoImposterSystem::Tick);
	}
}

void UProtoImposterSystem::Tick()
{
	TSet<Worker_EntityId_Key> NewPartitions;
	UWorld* World = GetWorld();
	if(!World->HasBegunPlay())
	{
		return;
	}

	if(WorldActor == nullptr)
	{
		WorldActor = World->SpawnActor<AScavengersHubIsmActor>();
		WorldActor->SetStaticMesh(ImposterMesh);
	}

	for (auto Event : ConsumeEntityEvents())
	{
		Worker_EntityId Entity = Event.PartitionId;
		switch(Event.Event)
		{
		case SpatialGDK::FEntityEvent::Created:
		//{
		//
		//}
		//break;
		case SpatialGDK::FEntityEvent::ToLowRes:
		{
			HighResActors.Remove(Entity);
		}
		break;
		case SpatialGDK::FEntityEvent::ToHiRes:
		{
			HighResActors.Add(Entity);
			TArray<float> CustomData;
			WorldActor->SetEntityTransform(Entity, TOptional<FTransform>(), CustomData, false);
		}
		break;
		case SpatialGDK::FEntityEvent::Deleted:
		{
			HighResActors.Remove(Entity);
			TArray<float> CustomData;
			WorldActor->SetEntityTransform(Entity, TOptional<FTransform>(), CustomData, false);
		}
		break;
		}
	}

	for(auto ToUpdate : Positions.GetModifiedEntities())
	{
		if (HighResActors.Contains(ToUpdate))
		{
			continue;
		}
		const FVector* Pos = Positions.GetPositions().Find(ToUpdate);
		if (!ensure(Pos != nullptr))
		{
			continue;
		}
		FTransform EntityTrans(*Pos);
		TArray<float> CustomData;
		WorldActor->SetEntityTransform(ToUpdate, EntityTrans, CustomData, false);
	}

	Positions.ClearModified();
	ActorMetadata.ClearModified();
}
