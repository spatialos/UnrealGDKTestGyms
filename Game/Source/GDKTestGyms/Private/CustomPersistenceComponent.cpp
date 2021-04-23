// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CustomPersistenceComponent.h"

#include "EngineClasses/SpatialGameInstance.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Utils/SpatialStatics.h"
#include "SpatialConstants.h"
#include <WorkerSDK/improbable/c_schema.h>

DEFINE_LOG_CATEGORY(LogCustomPersistence);

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
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("Owner invalid in BeginPlay."));
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("No SpatialNetDriver found in BeginPlay."));
		return;
	}

	// This assumes that BeginPlay will be called before the entity for the actor gets created
	if (GetOwnerRole() == ROLE_Authority)
	{
		NetDriver->OnActorEntityCreation(Owner).AddUObject(this, &UCustomPersistenceComponent::OnActorEntityCreated);
	}
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

// Once we gain authority, we know we have all the data for an entity, and authority to modify it.
// Check the CustomPersistence component for whether we've supplied the persistence data previously in this deployment or not.
// If not, grab the user data, hand it to the user code callback, and update the CustomPersistence component flag that we've supplied the data,
// so that we don't do so again when the actor migrates servers.
// Also register for the NetDriver callback for when the owner actor is getting replicated, so that we can add the component update provided by user code as well.
void UCustomPersistenceComponent::OnAuthorityGained()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("Owner actor invalid, can't apply persistence data."));
		return;
	}

	const uint64 EntityID = USpatialStatics::GetActorEntityId(Owner);
	if (EntityID == 0)
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("Owner actor has no entity ID, can't apply persistence data."));
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("No SpatialNetDriver found, can't apply persistence data."));
		return;
	}

	SpatialGDK::ViewCoordinator& Coordinator = NetDriver->Connection->GetCoordinator();
	if (!Coordinator.HasEntity(EntityID))
	{
		UE_LOG(LogCustomPersistence, Error,
			   TEXT("View coordinator doesn't have entity %llu, can't apply persistence data."), EntityID);
		return;
	}

	const SpatialGDK::EntityView& View = Coordinator.GetView();
	const SpatialGDK::EntityViewElement* ViewData = View.Find(EntityID);
	if (ViewData == nullptr)
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("Found no persistence data for entity %llu, can't apply persistence data."), EntityID);
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
		UE_LOG(LogCustomPersistence, Error, TEXT("Didn't find the custom persistence and/or user data component, can't apply persistence data."));
		return;
	}

	Schema_Object* CustomPersistenceFields = CustomPersistenceData->GetFields();
	const bool bSuppliedData = Schema_GetBool(CustomPersistenceFields, SpatialConstants::CUSTOM_PERSISTENCE_DATA_SUPPLIED_ID) != 0;
	if (!bSuppliedData)
	{
		if (!CVarPersistenceEnabled.GetValueOnGameThread())
		{
			UE_LOG(LogCustomPersistence, VeryVerbose, TEXT("Persistence disabled, not applying persistence data."));
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

	OnActorReplicationDelegateHandle = NetDriver->OnActorReplication(Owner).AddUObject(this, &UCustomPersistenceComponent::OnActorReplication);
}

void UCustomPersistenceComponent::OnAuthorityLost()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("Owner invalid in OnAuthorityLost."));
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogCustomPersistence, Error, TEXT("No SpatialNetDriver found in OnAuthorityLost."));
		return;
	}

	NetDriver->OnActorReplication(Owner).Remove(OnActorReplicationDelegateHandle);
}

void UCustomPersistenceComponent::OnActorReplication(TArray<SpatialGDK::ComponentUpdate>& OutComponentUpdates)
{
	// Will have to see if the ComponentUpdate type makes sense to be user-facing.
	SpatialGDK::ComponentUpdate Update(GetComponentId());
	GetComponentUpdate(Update);
	OutComponentUpdates.Emplace(MoveTemp(Update));
}

void UCustomPersistenceComponent::GetAddComponentData(SpatialGDK::ComponentData& Data) {}

void UCustomPersistenceComponent::GetComponentUpdate(SpatialGDK::ComponentUpdate& Update) {}

void UCustomPersistenceComponent::OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data) {}