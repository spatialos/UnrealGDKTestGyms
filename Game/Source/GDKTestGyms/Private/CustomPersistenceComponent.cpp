// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CustomPersistenceComponent.h"

#include "EngineClasses/SpatialGameInstance.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Utils/SpatialStatics.h"
#include "SpatialConstants.h"
#include <WorkerSDK/improbable/c_schema.h>

// Sets default values for this component's properties
UCustomPersistenceComponent::UCustomPersistenceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCustomPersistenceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!USpatialStatics::IsSpatialNetworkingEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("UCustomPersistenceComponent, not using spatial networking."));
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, didn't have an owner actor during BeginPlay."));
		return;
	}

	if (!IsValid(Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, Owner is not yet valid."));
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, running with spatial but can't find a spatial net driver."));
		return;
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		NetDriver->OnActorEntityCreation(Owner).AddUObject(this, &UCustomPersistenceComponent::OnActorEntityCreated);		
	}

	NetDriver->OnActorReplication(Owner).AddUObject(this, &UCustomPersistenceComponent::OnActorReplication);
}

void UCustomPersistenceComponent::OnActorEntityCreated(TArray<SpatialGDK::ComponentData>& OutComponentDatas)
{
	SpatialGDK::ComponentData CustomPersistenceData(SpatialConstants::CUSTOM_PERSISTENCE_COMPONENT_ID);
	Schema_Object* Fields = CustomPersistenceData.GetFields();
	Schema_AddBool(Fields, SpatialConstants::CUSTOM_PERSISTENCE_DATA_SUPPLIED_ID, true);
	OutComponentDatas.Emplace(MoveTemp(CustomPersistenceData));

	SpatialGDK::ComponentData UserData(GetComponentId());
	GetAddComponentData(UserData);
	OutComponentDatas.Emplace(MoveTemp(UserData));
}

void UCustomPersistenceComponent::OnActorReplication(TArray<SpatialGDK::ComponentUpdate>& OutComponentUpdates)
{
	// Will have to see if the ComponentUpdate type makes sense to be user-facing.
	SpatialGDK::ComponentUpdate Update(GetComponentId());
	GetComponentUpdate(Update);
	OutComponentUpdates.Emplace(MoveTemp(Update));
}

// Once we gain authority, we know we have all the data for an entity, and authority to modify it.
// If we can find data for the actor's persistence spatial component, pass it to the user implementation via OnPersistenceDataAvailable
// If we don't have any data for the component, add it to the entity. This usually happens when the actor is loaded for the first time in a
// fresh deployment, or spawned dynamically.
void UCustomPersistenceComponent::OnAuthorityGained()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, didn't have an owner actor during InternalOnPersistenceDataAvailable"));
		return;
	}

	const uint64 EntityID = USpatialStatics::GetActorEntityId(Owner);
	if (EntityID == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCustomPersistenceComponent, Don't have an entity ID in persistence callback."));
		return;
	}

	const USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, Got persistence data callback but can't find a spatial net driver."));
		return;
	}

	SpatialGDK::ViewCoordinator& Coordinator = NetDriver->Connection->GetCoordinator();

	if (!Coordinator.HasEntity(EntityID))
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("UCustomPersistenceComponent, View coordinator doesn't have entity %llu during persistence callback."), EntityID);
		return;
	}

	const SpatialGDK::EntityView& View = Coordinator.GetView();
	const SpatialGDK::EntityViewElement* ViewData = View.Find(EntityID);
	if (ViewData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, Found no persistence data for entity %llu"), EntityID);
		return;
	}

	const SpatialGDK::ComponentData* CustomPersistenceData = nullptr;
	const SpatialGDK::ComponentData* UserData = nullptr;

	for (const auto& ComponentData : ViewData->Components)
	{
		if (ComponentData.GetComponentId() == SpatialConstants::CUSTOM_PERSISTENCE_COMPONENT_ID)
		{
			CustomPersistenceData = &ComponentData;
		}
		
		if (ComponentData.GetComponentId() == GetComponentId())
		{
			UserData = &ComponentData;
		}
	}

	if (CustomPersistenceData == nullptr || UserData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCustomPersistenceComponent, OnAuthorityGained, either the custom persistence or user data component were missing from the entity."));
		return;
	}

	Schema_Object* CustomPersistenceFields = CustomPersistenceData->GetFields();
	const bool bSuppliedData = Schema_GetBool(CustomPersistenceFields, SpatialConstants::CUSTOM_PERSISTENCE_DATA_SUPPLIED_ID) != 0;
	if (!bSuppliedData)
	{
		if (!CVarPersistenceEnabled.GetValueOnGameThread())
		{
			UE_LOG(LogTemp, Log, TEXT("UCustomPersistenceComponent, Persistence disabled, not applying persistence data"));
		}
		else
		{
			OnPersistenceDataAvailable(*UserData);
		}

		// Set the flag signaling that we've supplied the persistence data
		SpatialGDK::ComponentUpdate CustomPersistenceUpdate(SpatialConstants::CUSTOM_PERSISTENCE_COMPONENT_ID);
		Schema_Object* UserDataFields = CustomPersistenceUpdate.GetFields();
		Schema_AddBool(UserDataFields, SpatialConstants::CUSTOM_PERSISTENCE_DATA_SUPPLIED_ID, true);
		Coordinator.SendComponentUpdate(EntityID, MoveTemp(CustomPersistenceUpdate), {});
	}
}

void UCustomPersistenceComponent::GetAddComponentData(SpatialGDK::ComponentData& Data) {}

void UCustomPersistenceComponent::GetComponentUpdate(SpatialGDK::ComponentUpdate& Update) {}

void UCustomPersistenceComponent::OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data) {}