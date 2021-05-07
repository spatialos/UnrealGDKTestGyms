// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SchemaPersistence/SchemaPersistenceComponent.h"

#include "EngineClasses/SpatialGameInstance.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "SpatialConstants.h"
#include "Utils/SpatialStatics.h"
#include <WorkerSDK/improbable/c_schema.h>

DEFINE_LOG_CATEGORY(LogSchemaPersistence);

static TAutoConsoleVariable<bool> CVarPersistenceEnabled(
	TEXT("spatial.SchemaPersistenceEnabled"),
	true,
	TEXT("Determines if persistence data should be applied.\n"),
	ECVF_Cheat);

const Worker_ComponentId SCHEMA_PERSISTENCE_ID = 1000;
const Schema_FieldId SCHEMA_PERSISTENCE_DATA_SUPPLIED = 1;

// Sets default values for this component's properties
USchemaPersistenceComponent::USchemaPersistenceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USchemaPersistenceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!USpatialStatics::IsSpatialNetworkingEnabled())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("Owner actor %s is invalid in BeginPlay."), *GetNameSafe(Owner));
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("No SpatialNetDriver found in BeginPlay."));
		return;
	}

	// This assumes that BeginPlay will be called before the entity for the actor gets created
	if (GetOwnerRole() == ROLE_Authority)
	{
		NetDriver->OnActorEntityCreation(Owner).AddUObject(this, &USchemaPersistenceComponent::OnActorEntityCreated);
	}
}

void USchemaPersistenceComponent::OnActorEntityCreated(TArray<SpatialGDK::ComponentData>& OutComponentDatas)
{
	UE_LOG(LogTemp, Log, TEXT("entity recreation / %s / OnActorEntityCreated"), *GetOwner()->GetName());
	
	SpatialGDK::ComponentData CustomPersistenceData(SCHEMA_PERSISTENCE_ID);
	Schema_Object* Fields = CustomPersistenceData.GetFields();
	Schema_AddBool(Fields, SCHEMA_PERSISTENCE_DATA_SUPPLIED, true);
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
void USchemaPersistenceComponent::OnAuthorityGained()
{
	if (!USpatialStatics::IsSpatialNetworkingEnabled())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("Owner actor %s is invalid, can't apply persistence data."), *GetNameSafe(Owner));
		return;
	}

	const uint64 EntityID = USpatialStatics::GetActorEntityId(Owner);
	if (EntityID == 0)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("Owner actor %s has no entity ID, can't apply persistence data."), *Owner->GetName());
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("No SpatialNetDriver found, can't apply persistence data."));
		return;
	}

	SpatialGDK::ViewCoordinator& Coordinator = NetDriver->Connection->GetCoordinator();
	if (!Coordinator.HasEntity(EntityID))
	{
		UE_LOG(LogSchemaPersistence, Error,
			TEXT("View coordinator doesn't have entity %llu (actor %s), can't apply persistence data."), EntityID, *Owner->GetName());
		return;
	}

	const SpatialGDK::EntityView& View = Coordinator.GetView();
	const SpatialGDK::EntityViewElement* ViewData = View.Find(EntityID);
	if (ViewData == nullptr)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("Found no persistence data for entity %llu (actor %s), can't apply persistence data."), EntityID, *Owner->GetName());
		return;
	}

	const SpatialGDK::ComponentData* CustomPersistenceData = nullptr;
	const SpatialGDK::ComponentData* UserData = nullptr;

	for (const auto& ComponentData : ViewData->Components)
	{
		if (ComponentData.GetComponentId() == SCHEMA_PERSISTENCE_ID)
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
		UE_LOG(LogSchemaPersistence, Error, TEXT("Didn't find the custom persistence and/or user data component on entity %llu (actor %s), can't apply persistence data."), EntityID, *Owner->GetName());
		return;
	}

	Schema_Object* CustomPersistenceFields = CustomPersistenceData->GetFields();
	const bool bSuppliedData = Schema_GetBool(CustomPersistenceFields, SCHEMA_PERSISTENCE_DATA_SUPPLIED) != 0;
	if (!bSuppliedData)
	{
		if (!CVarPersistenceEnabled.GetValueOnGameThread())
		{
			UE_LOG(LogSchemaPersistence, VeryVerbose, TEXT("Persistence disabled, not applying persistence data."));
		}
		else
		{
			OnPersistenceDataAvailable(*UserData);
		}

		// Set the flag signaling that we've supplied the persistence data
		SpatialGDK::ComponentUpdate CustomPersistenceUpdate(SCHEMA_PERSISTENCE_ID);
		Schema_Object* UserDataFields = CustomPersistenceUpdate.GetFields();
		Schema_AddBool(UserDataFields, SCHEMA_PERSISTENCE_DATA_SUPPLIED, true);
		Coordinator.SendComponentUpdate(EntityID, MoveTemp(CustomPersistenceUpdate), {});
	}

	OnActorReplicationDelegateHandle = NetDriver->OnActorReplication(Owner).AddUObject(this, &USchemaPersistenceComponent::OnActorReplication);
}

void USchemaPersistenceComponent::OnAuthorityLost()
{
	RemoveActorReplicationDelegate();
}

void USchemaPersistenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveActorReplicationDelegate();
}

void USchemaPersistenceComponent::RemoveActorReplicationDelegate()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("Owner actor %s is invalid in RemoveActorReplicationDelegate."), *GetNameSafe(Owner));
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("Owner didn't have a world in RemoveActorReplicationDelegate"));
		return;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogSchemaPersistence, Error, TEXT("No SpatialNetDriver found in RemoveActorReplicationDelegate."));
		return;
	}

	NetDriver->OnActorReplication(Owner).Remove(OnActorReplicationDelegateHandle);
}

void USchemaPersistenceComponent::OnActorReplication(TArray<SpatialGDK::ComponentUpdate>& OutComponentUpdates)
{
	// Will have to see if the ComponentUpdate type makes sense to be user-facing.
	SpatialGDK::ComponentUpdate Update(GetComponentId());
	GetComponentUpdate(Update);
	OutComponentUpdates.Emplace(MoveTemp(Update));
}

void USchemaPersistenceComponent::GetAddComponentData(SpatialGDK::ComponentData& Data)
{
}

void USchemaPersistenceComponent::GetComponentUpdate(SpatialGDK::ComponentUpdate& Update)
{
}

void USchemaPersistenceComponent::OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data)
{
}
