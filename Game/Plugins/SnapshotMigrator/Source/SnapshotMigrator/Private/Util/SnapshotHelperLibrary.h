// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreTypes.h"

#include <WorkerSDK/improbable/c_schema.h>
#include <WorkerSDK/improbable/c_worker.h>

#include "Schema/UnrealObjectRef.h"

#include "SchemaBundleWrappers.h"
#include "SpatialCommonTypes.h"

#include <functional>

class SnapshotHelperLibrary
{
public:
	/**
	* Checks to see if a given snapshot stream is still usable.
	* Keeps us from having to repeat the contained boilerplate a) every time we want to check a Snapshot stream's status and b) for both input and output streams
	*	@param	GetStreamState		Pointer to function that accepts a TStream and returns a Worker_SnapshotState struct
	*	@param	StreamPtr			Pointer to the stream we want to operate on
	*	@param	OpContext			Description of what operation we performed just prior to doing this check. Used to output descriptive error messages if the stream is in a bad state.
	*
	*	@return						True if the stream is still valid and usable, False otherwise.
	*/
	template <typename TStream>
	static bool IsStreamStateValid(Worker_SnapshotState (*GetStreamState)(TStream*), TStream* StreamPtr, const FString& OpContext)
	{
		const Worker_SnapshotState State = GetStreamState(StreamPtr);
		if (State.stream_state != Worker_StreamState::WORKER_STREAM_STATE_GOOD)
		{
			UE_LOG(LogSnapshotMigrator, Warning, TEXT("Failed to %s: %s"), *OpContext, UTF8_TO_TCHAR(State.error_message));
			return false;
		}

		return true;
	}

	static const worker::c::Worker_ComponentData* GetComponentFromEntityById(const Worker_Entity* Entity, const Worker_ComponentId ComponentId);
};

class SnapshotDataMigrator
{
public:
	SnapshotDataMigrator(const SchemaBundleDefinitions& InOldDefinitions, const SchemaBundleDefinitions& InNewDefinitions) :
		OldDefinitions(InOldDefinitions), NewDefinitions(InNewDefinitions)
	{
	}
	~SnapshotDataMigrator()
	{
	}

	bool MigratePrimitiveField(const SchemaBundleFieldDefinition::SchemaPrimitiveType PrimitiveType, const Schema_FieldId OldId, const Schema_FieldId NewId, Schema_Object* OldSchemaObject, Schema_Object* NewSchemaObject);
	bool MigrateObjectField(const FString& ObjectType, const Schema_FieldId OldId, const Schema_FieldId NewId, Schema_Object* OldSchemaObject, Schema_Object* NewSchemaObject);

private:
	const SchemaBundleDefinitions& OldDefinitions;
	const SchemaBundleDefinitions& NewDefinitions;

	const FString COORDINATES_TYPE{ TEXT("improbable.Coordinates") };
	const FString VECTOR_TYPE{ TEXT("unreal.Vector3f") };
	const FString ROTATOR_TYPE{ TEXT("unreal.Rotator") };
	const FString UNREAL_OBJECT_REF_TYPE{ TEXT("unreal.UnrealObjectRef") };

private:
	template <typename T>
	using SchemaExtractor = std::function<T(const Schema_Object*, Schema_FieldId, uint32_t)>;
	using SchemaCounter = std::function<uint32_t(const Schema_Object*, Schema_FieldId)>;
	template <typename T>
	using SchemaAdder = std::function<void(Schema_Object*, Schema_FieldId, T Value)>;
	template <typename T>
	using SchemaTransformer = std::function<void(T&)>;

	template <typename T>
	struct SchemaFunctions
	{
		SchemaExtractor<T> Extract;
		SchemaCounter Count;
		SchemaAdder<T> Add;
		SchemaTransformer<T> Transform = [](T& Value) {};
	};

	template <typename T>
	static bool Migrate_Internal(const SchemaFunctions<T>& SchemaFunctions, const Schema_FieldId OldId, const Schema_FieldId NewId, Schema_Object* OldSchemaObject, Schema_Object* NewSchemaObject)
	{
		TArray<T> Container;

		const uint32 NumToMigrate = SchemaFunctions.Count(OldSchemaObject, OldId);
		if (NumToMigrate == 0)
		{
			return false;
		}

		Container.Reserve(NumToMigrate);

		for (uint32 i = 0; i < NumToMigrate; i++)
		{
			Container.Add(SchemaFunctions.Extract(OldSchemaObject, OldId, i));
		}

		for (T Value : Container)
		{
			SchemaFunctions.Transform(Value);
			SchemaFunctions.Add(NewSchemaObject, NewId, Value);
		}

		return true;
	}

	// Helper for object extraction. Wraps SpatialGDK::IndexYFromSchema functions.
	template <typename T>
	static SchemaExtractor<T> WrapGDKObjectExtractor(T (*GDKExtractor)(Schema_Object*, Schema_FieldId, uint32))
	{
		return [GDKExtractor](const Schema_Object* Object, Schema_FieldId FieldId, uint32_t Index) {
			// const_cast is safe because we're originally passing a non-const Schema_Object to MigrateObjectField.
			return GDKExtractor(const_cast<Schema_Object*>(Object), FieldId, Index);
		};
	}

private:
	void PatchUnrealObjectRef(FUnrealObjectRef& UnrealObjectRef);
};
