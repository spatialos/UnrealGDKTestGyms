#include "SnapshotHelperLibrary.h"

#include "Schema/Interest.h"
#include "Schema/StandardLibrary.h"
#include "Utils/SchemaUtils.h"

const worker::c::Worker_ComponentData* SnapshotHelperLibrary::GetComponentFromEntityById(const Worker_Entity* Entity, const Worker_ComponentId ComponentId)
{
	for (uint32 i = 0; i < Entity->component_count; i++)
	{
		if (Entity->components[i].component_id == ComponentId)
		{
			return &Entity->components[i];
		}
	}

	return nullptr;
}

bool SnapshotDataMigrator::MigratePrimitiveField(const SchemaBundleFieldDefinition::SchemaPrimitiveType PrimitiveType, const Schema_FieldId OldId, const Schema_FieldId NewId, Schema_Object* OldSchemaObject, Schema_Object* NewSchemaObject)
{
	switch (PrimitiveType)
	{
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::Int32:
		{
			const SchemaFunctions<int32_t> Funcs{
				Schema_IndexInt32,
				Schema_GetInt32Count,
				Schema_AddInt32
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::Uint64:
		{
			const SchemaFunctions<uint64_t> Funcs{
				Schema_IndexUint64,
				Schema_GetUint64Count,
				Schema_AddUint64
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::Uint32:
		{
			const SchemaFunctions<uint32_t> Funcs{
				Schema_IndexUint32,
				Schema_GetUint32Count,
				Schema_AddUint32
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::Bool:
		{
			const SchemaFunctions<uint8_t> Funcs{
				Schema_IndexBool,
				Schema_GetBoolCount,
				Schema_AddBool
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::Float:
		{
			const SchemaFunctions<float> Funcs{
				Schema_IndexFloat,
				Schema_GetFloatCount,
				Schema_AddFloat
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}

		case SchemaBundleFieldDefinition::SchemaPrimitiveType::String:
		{
			auto Adder = [](Schema_Object* Object, Schema_FieldId FieldId, FString String) {
				SpatialGDK::AddStringToSchema(Object, FieldId, String);
			};

			const SchemaFunctions<FString> Funcs{
				SpatialGDK::IndexStringFromSchema,
				Schema_GetBytesCount,
				Adder
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::Bytes:
		{
			auto Adder = [](Schema_Object* Object, Schema_FieldId FieldId, TArray<uint8> Bytes) {
				SpatialGDK::AddBytesToSchema(Object, FieldId, Bytes.GetData(), Bytes.Num());
			};

			const SchemaFunctions<TArray<uint8>> Funcs{
				SpatialGDK::IndexBytesFromSchema,
				Schema_GetBytesCount,
				Adder
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		case SchemaBundleFieldDefinition::SchemaPrimitiveType::EntityId:
		{
			const SchemaFunctions<Worker_EntityId> Funcs{
				Schema_IndexEntityId,
				Schema_GetEntityIdCount,
				Schema_AddEntityId
			};

			return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
		}
		default:
			checkNoEntry();
			return false;
	}
}

bool SnapshotDataMigrator::MigrateObjectField(const FString& ObjectType, const Schema_FieldId OldId, const Schema_FieldId NewId, Schema_Object* OldSchemaObject, Schema_Object* NewSchemaObject)
{
	if (ObjectType == UNREAL_OBJECT_REF_TYPE)
	{
		auto Adder = [](Schema_Object* Object, Schema_FieldId FieldId, FUnrealObjectRef ObjectRef) {
			if (ObjectRef.Entity == SpatialConstants::INVALID_ENTITY_ID)
			{
				return;
			}

			SpatialGDK::AddObjectRefToSchema(Object, FieldId, ObjectRef);
		};

		const SchemaFunctions<FUnrealObjectRef> Funcs{
			WrapGDKObjectExtractor(SpatialGDK::IndexObjectRefFromSchema),
			Schema_GetObjectCount,
			Adder,
			[&](FUnrealObjectRef& UnrealObjRef) { PatchUnrealObjectRef(UnrealObjRef); }
		};

		return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
	}
	else if (ObjectType == ROTATOR_TYPE)
	{
		const SchemaFunctions<FRotator> Funcs{
			WrapGDKObjectExtractor(SpatialGDK::IndexRotatorFromSchema),
			Schema_GetObjectCount,
			SpatialGDK::AddRotatorToSchema
		};

		return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
	}
	else if (ObjectType == VECTOR_TYPE)
	{
		const SchemaFunctions<FVector> Funcs{
			WrapGDKObjectExtractor(SpatialGDK::IndexVectorFromSchema),
			Schema_GetObjectCount,
			SpatialGDK::AddVectorToSchema
		};

		return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
	}
	else if (ObjectType == COORDINATES_TYPE)
	{
		auto Adder = [](Schema_Object* Object, Schema_FieldId Id, SpatialGDK::Coordinates Coordinate) {
			SpatialGDK::AddCoordinateToSchema(Object, Id, Coordinate);
		};

		const SchemaFunctions<SpatialGDK::Coordinates> Funcs{
			WrapGDKObjectExtractor(SpatialGDK::IndexCoordinateFromSchema),
			Schema_GetObjectCount,
			Adder
		};

		return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
	}
	else if (ObjectType == SchemaBundleFieldDefinition::COMPONENT_SET_INTEREST_MAP)
	{
		auto Extractor = [](const Schema_Object* Object, Schema_FieldId Id, uint32 Index) {
			Schema_Object* ComponentInterestPairObject = Schema_IndexObject(const_cast<Schema_Object*>(Object), Id, Index);
			uint32 Key = Schema_GetUint32(ComponentInterestPairObject, SCHEMA_MAP_KEY_FIELD_ID);
			SpatialGDK::ComponentSetInterest Value = SpatialGDK::GetComponentInterestFromSchema(ComponentInterestPairObject, SCHEMA_MAP_VALUE_FIELD_ID);
			return TPair<uint32, SpatialGDK::ComponentSetInterest>{ Key, Value };
		};

		auto Adder = [](Schema_Object* Object, Schema_FieldId Id, TPair<uint32, SpatialGDK::ComponentSetInterest> ComponentInterestPair) {
			if (ComponentInterestPair.Key == SpatialConstants::INVALID_COMPONENT_ID)
			{
				return;
			}

			Schema_Object* ComponentInterestPairObject = Schema_AddObject(Object, Id);
			Schema_AddUint32(ComponentInterestPairObject, SCHEMA_MAP_KEY_FIELD_ID, ComponentInterestPair.Key);
			SpatialGDK::AddComponentInterestToInterestSchema(ComponentInterestPairObject, SCHEMA_MAP_VALUE_FIELD_ID, ComponentInterestPair.Value);
		};

		const SchemaFunctions<TPair<uint32, SpatialGDK::ComponentSetInterest>> Funcs{
			Extractor,
			Schema_GetObjectCount,
			Adder
		};

		return Migrate_Internal(Funcs, OldId, NewId, OldSchemaObject, NewSchemaObject);
	}

	return false;
}

void SnapshotDataMigrator::PatchUnrealObjectRef(FUnrealObjectRef& UnrealObjectRef)
{
	const Worker_ComponentId OldOffset = UnrealObjectRef.Offset;
	if (OldOffset > SpatialConstants::INVALID_COMPONENT_ID)
	{
		Worker_ComponentId NewOffset;
		if (!SchemaBundleDefinitions::GetCorrespondingComponentId(OldDefinitions, NewDefinitions, OldOffset, NewOffset))
		{
			UnrealObjectRef.Entity = SpatialConstants::INVALID_ENTITY_ID;
			UnrealObjectRef.Offset = SpatialConstants::INVALID_COMPONENT_ID;
			return;
		}

		UnrealObjectRef.Offset = NewOffset;
	}

	if (UnrealObjectRef.Outer)
	{
		PatchUnrealObjectRef(UnrealObjectRef.Outer.GetValue());
	}
}
