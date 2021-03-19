// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotStreamHelpers.h"
#include "SnapshotMigratorModule.h"

InputSnapshotHelper::InputSnapshotHelper(const FString& Path) :
	SnapshotHelper<Worker_SnapshotInputStream>(Path, Worker_SnapshotInputStream_Create, Worker_SnapshotInputStream_GetState, Worker_SnapshotInputStream_Destroy)
{
}

InputSnapshotHelper::~InputSnapshotHelper()
{
	for (Worker_ComponentData* AcquiredComponentDatum : AcquiredComponentData)
	{
		Worker_ReleaseComponentData(AcquiredComponentDatum);
	}
}

bool InputSnapshotHelper::ReadEntities(Entities& OutEntities)
{
	while (Worker_SnapshotInputStream_HasNext(Stream))
	{
		if (!IsStreamValid())
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Snapshot input stream no longer valid after calling HasNext!"));
			return false;
		}

		const Worker_Entity* Entity = Worker_SnapshotInputStream_ReadEntity(Stream);
		if (!IsStreamValid())
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Snapshot input stream no longer valid after calling ReadEntity!"));
			return false;
		}

		TArray<Worker_ComponentData> EntityComponents;

		for (uint32 i = 0; i < Entity->component_count; ++i)
		{
			// We need to call this so that we have a locally managed copy of the component data
			// Otherwise, the next ReadEntity call will invalidate things.
			Worker_ComponentData* AcquiredComponentDatum = Worker_AcquireComponentData(&Entity->components[i]);
			AcquiredComponentData.Add(AcquiredComponentDatum);
			EntityComponents.Add(*AcquiredComponentDatum);
		}

		OutEntities.Add(Entity->entity_id, EntityComponents);
	}

	return true;
}

OutputSnapshotHelper::OutputSnapshotHelper(const FString& Path) :
	SnapshotHelper<Worker_SnapshotOutputStream>(Path, Worker_SnapshotOutputStream_Create, Worker_SnapshotOutputStream_GetState, Worker_SnapshotOutputStream_Destroy)
{
}

bool OutputSnapshotHelper::WriteEntitiesToSnapshot(const Entities& EntitiesToWrite)
{
	// Not strictly necessary, but this helps make diffing text translations of pre/post-migration snapshots easier.
	TArray<Worker_EntityId> SortedEntityIds;
	EntitiesToWrite.GenerateKeyArray(SortedEntityIds);
	SortedEntityIds.Sort();

	for (auto EntityId : SortedEntityIds)
	{
		Worker_Entity NewEntity;
		NewEntity.entity_id = EntityId;
		const Components& EntityComponents = EntitiesToWrite.FindChecked(EntityId);

		NewEntity.components = EntityComponents.GetData();
		NewEntity.component_count = EntityComponents.Num();

		Worker_SnapshotOutputStream_WriteEntity(Stream, &NewEntity);
		if (!IsStreamValid())
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Snapshot output stream no longer valid after calling WriteEntity!"));
			return false;
		}
	}

	return true;
}
