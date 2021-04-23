// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "ResourceNodePersistenceComponent.h"


#include <WorkerSDK/improbable/c_schema.h>

#include "CustomSchemaSnapshotActor.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "Interop/Connection/SpatialWorkerConnection.h"

const Schema_FieldId DEPLETED_FIELD_ID = 1;

UResourceNodePersistenceComponent::UResourceNodePersistenceComponent()
{
}

void UResourceNodePersistenceComponent::GetAddComponentData(SpatialGDK::ComponentData& Data)
{
	Schema_Object* Fields = Data.GetFields();
	Schema_AddBool(Fields, DEPLETED_FIELD_ID, false);
}

void UResourceNodePersistenceComponent::GetComponentUpdate(SpatialGDK::ComponentUpdate& Update)
{
	AActor* Owner = GetOwner();
	FResourceNodePersistenceData Data;
	GetBlueprintUpdateData(Data); // Note that this function completely overwrites `Data`.
	// Data from C++ would be added to the struct after the call to blueprint code, or added to the Schema_Object directly.

	// Old code (stores mirrors of the persistence data on the actor component)
	// ACustomSchemaSnapshotActor* SnapshotActor = Cast<ACustomSchemaSnapshotActor>(Owner);
	// bDepleted = SnapshotActor->bDepleted; // Copy over our data. This is very prone to just forgetting to set the field on this component :/

	Schema_Object* Fields = Update.GetFields();
	Schema_AddBool(Fields, DEPLETED_FIELD_ID, Data.bDepleted);
}

void UResourceNodePersistenceComponent::OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data)
{
	UE_LOG(LogTemp, Log, TEXT("UResourceNodePersistenceComponent, OnPersistenceDataAvailable. Worker: %s"), *Cast<USpatialNetDriver>(GetWorld()->GetNetDriver())->Connection->GetWorkerId());
	Schema_Object* Fields = Data.GetFields();

	FResourceNodePersistenceData PersistenceData;
	PersistenceData.bDepleted = Schema_GetBool(Fields, DEPLETED_FIELD_ID) != 0;
	ApplyBlueprintPersistenceData(PersistenceData);
	// Could also read out and apply data directly in C++ code here.

	// Old code (stores mirrors of the persistence data on the actor component)
	// UE_LOG(LogTemp, Log, TEXT("UResourceNodePersistenceComponent, depleted before persistence: %d"), bDepleted);
	// bDepleted = Schema_GetBool(Fields, DEPLETED_FIELD_ID) != 0;
	// UE_LOG(LogTemp, Log, TEXT("UResourceNodePersistenceComponent, depleted after persistence: %d"), bDepleted);
}
