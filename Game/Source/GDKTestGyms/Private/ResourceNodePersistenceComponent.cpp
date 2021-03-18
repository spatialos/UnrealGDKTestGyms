// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "ResourceNodePersistenceComponent.h"


#include <WorkerSDK/improbable/c_schema.h>
#include <WorkerSDK/improbable/c_worker.h>

const Worker_ComponentId PERSISTENCE_COMPONENT_ID = 9950;
const Schema_FieldId DEPLETED_FIELD_ID = 1;


UResourceNodePersistenceComponent::UResourceNodePersistenceComponent()
{
}

void UResourceNodePersistenceComponent::GetComponentUpdate(SpatialGDK::ComponentUpdate& Update)
{
	Schema_Object* Fields = Update.GetFields();
	Schema_AddBool(Fields, DEPLETED_FIELD_ID, bDepleted);
}

void UResourceNodePersistenceComponent::OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data)
{
	Schema_Object* Fields = Data.GetFields();
	UE_LOG(LogTemp, Log, TEXT("depleted before persistence: %d"), bDepleted);
	bDepleted = Schema_GetBool(Fields, DEPLETED_FIELD_ID) != 0;
	UE_LOG(LogTemp, Log, TEXT("depleted after persistence: %d"), bDepleted);
}
