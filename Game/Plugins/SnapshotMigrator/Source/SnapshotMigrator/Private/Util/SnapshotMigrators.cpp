// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotMigrators.h"

#include "Engine/World.h"
#include "EngineClasses/SpatialActorChannel.h"
#include "EngineClasses/SpatialNetConnection.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialPackageMapClient.h"
#include "Internationalization/Regex.h"
#include "Misc/ScopeExit.h"
#include "SnapshotMigratorModule.h"
#include "Tests/SnapshotMigrationSettings.h"
#include "Utils/ComponentFactory.h"
#include "Utils/EntityFactory.h"
#include "WorkerSDK/improbable/c_schema.h"

namespace
{
// Mostly copied from AutomationCommon::GetAnyGameWorld(), except we explicitly want the UWorld that represents the server
UWorld* GetAnyGameWorldServer()
{
	UWorld* World = nullptr;
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && (Context.World() != nullptr) && (Context.World()->GetNetMode() == NM_DedicatedServer))
		{
			World = Context.World();
			break;
		}
	}

	return World;
}

ULevel* FindLevelByName(const UWorld* World, const FString& LevelName)
{
	for (ULevel* Level : World->GetLevels())
	{
		if (Level->GetName() == LevelName)
		{
			return Level;
		}
	}

	return nullptr;
}

USpatialNetDriver* GetSpatialNetDriver()
{
	UWorld* World = GetAnyGameWorldServer();
	check(World);
	return CastChecked<USpatialNetDriver>(World->GetNetDriver());
}
}	 // namespace

FieldMigrator::FieldMigrator(const TSharedPtr<SchemaBundleDefinitions>& InOldSchemaDefinitions, const TSharedPtr<SchemaBundleDefinitions>& InNewSchemaDefinitions) :
	OldSchemaDefinitions(InOldSchemaDefinitions),
	NewSchemaDefinitions(InNewSchemaDefinitions)
{
	DataMigrator = MakeUnique<SnapshotDataMigrator>(*OldSchemaDefinitions, *NewSchemaDefinitions);
}

bool FieldMigrator::ShouldMigrateField(const Worker_ComponentData* OldComponent, const SchemaBundleFieldDefinition& OldFieldDefinition, Worker_ComponentUpdate& OutComponentUpdate, const SchemaBundleFieldDefinition& NewFieldDefinition)
{
	Schema_Object* OldComponentSchemaObject = Schema_GetComponentDataFields(OldComponent->schema_type);
	Schema_Object* UpdateSchemaObject = Schema_GetComponentUpdateFields(OutComponentUpdate.schema_type);

	Schema_FieldId OldFieldId = OldFieldDefinition.GetId();
	Schema_FieldId NewFieldId = NewFieldDefinition.GetId();

	bool bMigratedSomething = false;

	if (NewFieldDefinition.IsMap())
	{
		// Right now we only support ComponentSetInterest maps, which are normally a field on the Interest component.
		if (NewFieldDefinition.IsPrimitive(SchemaBundleFieldDefinition::TypeIndex::KEY) && NewFieldDefinition.GetPrimitiveType(SchemaBundleFieldDefinition::TypeIndex::KEY) == SchemaBundleFieldDefinition::SchemaPrimitiveType::Uint32)
		{
			if (NewFieldDefinition.IsType(SchemaBundleFieldDefinition::TypeIndex::VALUE) && NewFieldDefinition.GetResolvedType(SchemaBundleFieldDefinition::TypeIndex::VALUE).Equals(FString{ TEXT("improbable.ComponentSetInterest") }))
			{
				bMigratedSomething = DataMigrator->MigrateObjectField(SchemaBundleFieldDefinition::COMPONENT_SET_INTEREST_MAP, OldFieldId, NewFieldId, OldComponentSchemaObject, UpdateSchemaObject);
			}
		}
	}
	else
	{
		if (NewFieldDefinition.IsPrimitive())
		{
			bMigratedSomething = DataMigrator->MigratePrimitiveField(NewFieldDefinition.GetPrimitiveType(), OldFieldId, NewFieldId, OldComponentSchemaObject, UpdateSchemaObject);
		}
		else
		{
			bMigratedSomething = DataMigrator->MigrateObjectField(NewFieldDefinition.GetResolvedType(), OldFieldId, NewFieldId, OldComponentSchemaObject, UpdateSchemaObject);
		}
	}

	if (!NewFieldDefinition.IsSingular() && !bMigratedSomething)
	{
		Schema_AddComponentUpdateClearedField(OutComponentUpdate.schema_type, NewFieldId);
		bMigratedSomething = true;
	}

	return bMigratedSomething;
}

ComponentMigrator::ComponentMigrator(const TSharedPtr<SchemaBundleDefinitions>& InOldSchemaDefinitions, const TSharedPtr<SchemaBundleDefinitions>& InNewSchemaDefinitions) :
	OldSchemaDefinitions(InOldSchemaDefinitions),
	NewSchemaDefinitions(InNewSchemaDefinitions)
{
	FieldMigratorPtr = MakeUnique<FieldMigrator>(OldSchemaDefinitions, NewSchemaDefinitions);
}

bool ComponentMigrator::MigrateComponent(const Worker_ComponentData* OldComponent, Worker_ComponentData& OutNewComponent, FComponentMigrationRecord& ComponentMigrationRecord)
{
	Worker_ComponentUpdate Update;
	Update.component_id = OutNewComponent.component_id;
	Update.schema_type = Schema_CreateComponentUpdate();
	ON_SCOPE_EXIT
	{
		Schema_DestroyComponentUpdate(Update.schema_type);
	};

	if (!ShouldUpdateComponent(OldComponent, OutNewComponent.component_id, Update, ComponentMigrationRecord))
	{
		return true;
	}

	Schema_Object* Obj = Schema_GetComponentUpdateFields(Update.schema_type);

	TArray<Schema_FieldId> AlteredFieldIds;
	AlteredFieldIds.SetNumUninitialized(Schema_GetUniqueFieldIdCount(Obj));
	Schema_GetUniqueFieldIds(Obj, AlteredFieldIds.GetData());

	TArray<Schema_FieldId> ClearedFieldIds;
	ClearedFieldIds.SetNumUninitialized(Schema_GetComponentUpdateClearedFieldCount(Update.schema_type));
	Schema_GetComponentUpdateClearedFieldList(Update.schema_type, ClearedFieldIds.GetData());

	for (const Schema_FieldId FieldId : ClearedFieldIds)
	{
		Schema_ClearField(Schema_GetComponentDataFields(OutNewComponent.schema_type), FieldId);
	}

	const uint8_t ApplyResult = Schema_ApplyComponentUpdateToData(Update.schema_type, OutNewComponent.schema_type);
	if (ApplyResult == 0)
	{
		const FString Error{ UTF8_TO_TCHAR(Schema_GetError(Schema_GetComponentDataFields(OutNewComponent.schema_type))) };
		UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to migrate data forward onto component %ld: %s"), OutNewComponent.component_id, *Error);
		return false;
	}

	return true;
}

bool ComponentMigrator::ShouldUpdateComponent(const Worker_ComponentData* OldComponent, const Worker_ComponentId NewComponentId, Worker_ComponentUpdate& OutComponentUpdate, FComponentMigrationRecord& ComponentMigrationRecord)
{
	bool bWroteUpdate = false;

	for (const SchemaBundleFieldDefinition& FieldDefinition : NewSchemaDefinitions->FindComponentChecked(NewComponentId).GetFields())
	{
		FFieldMigrationRecord& FieldMigrationRecord = ComponentMigrationRecord.FieldMigrationRecords.AddDefaulted_GetRef();
		FieldMigrationRecord.TargetFieldId = FieldDefinition.GetId();
		FieldMigrationRecord.FieldName = FieldDefinition.GetName();
		// Since most branches below skip, assume skipped unless we manage to migrate the field and receive an indicator that something was modified,
		// at which point we'll instead set success
		FieldMigrationRecord.Result = EMigrationResult::Skipped;

		const SchemaBundleFieldDefinition* OldFieldDefinition = OldSchemaDefinitions->FindComponentChecked(OldComponent->component_id).FindField(FieldDefinition.GetName());
		if (OldFieldDefinition == nullptr)
		{
			FieldMigrationRecord.ResultReason = TEXT("Source field not found");
			continue;
		}

		FieldMigrationRecord.SourceFieldId = OldFieldDefinition->GetId();

		if (!OldFieldDefinition->IsSameTypeAs(FieldDefinition))
		{
			if (!(OldFieldDefinition->IsMap() || FieldDefinition.IsMap()))
			{
				FieldMigrationRecord.ResultReason = FString::Printf(TEXT("Non-map Field type mismatch. Cardinality: (%d,%d) (%d,%d) (%d,%d) (%d,%d) PrimitiveTypeMatch: (%d,%d) IsEnum: (%d,%d), IsType: (%d,%d) ResolvedType: (%s,%s)"),
					OldFieldDefinition->IsSingular(), FieldDefinition.IsSingular(),
					OldFieldDefinition->IsOptional(), FieldDefinition.IsOptional(),
					OldFieldDefinition->IsList(), FieldDefinition.IsList(),
					OldFieldDefinition->IsMap(), FieldDefinition.IsMap(),

					OldFieldDefinition->GetPrimitiveType(), FieldDefinition.GetPrimitiveType(),
					OldFieldDefinition->IsEnum(), FieldDefinition.IsEnum(),
					OldFieldDefinition->IsType(), FieldDefinition.IsType(),
					*OldFieldDefinition->GetResolvedType(), *FieldDefinition.GetResolvedType());
			}
			else
			{
				const SchemaBundleFieldDefinition::TypeIndex FieldId = SchemaBundleFieldDefinition::TypeIndex::VALUE;

				FieldMigrationRecord.ResultReason = FString::Printf(TEXT("Map Field type mismatch. Cardinality: (%d,%d) (%d,%d) (%d,%d) (%d,%d) PrimitiveTypeMatch: (%d,%d) IsEnum: (%d,%d), IsType: (%d,%d) ResolvedType: (%s,%s)"),
					OldFieldDefinition->IsSingular(), FieldDefinition.IsSingular(),
					OldFieldDefinition->IsOptional(), FieldDefinition.IsOptional(),
					OldFieldDefinition->IsList(), FieldDefinition.IsList(),
					OldFieldDefinition->IsMap(), FieldDefinition.IsMap(),

					OldFieldDefinition->GetPrimitiveType(FieldId), FieldDefinition.GetPrimitiveType(FieldId),
					OldFieldDefinition->IsEnum(FieldId), FieldDefinition.IsEnum(FieldId),
					OldFieldDefinition->IsType(FieldId), FieldDefinition.IsType(FieldId),
					*OldFieldDefinition->GetResolvedType(FieldId), *FieldDefinition.GetResolvedType(FieldId));
			}

			// Todo: Allow some type->type migrations either implicitly (i.e., int to float) or as defined by userspace code
			continue;
		}

		const bool bMigratedSomething = FieldMigratorPtr->ShouldMigrateField(OldComponent, *OldFieldDefinition, OutComponentUpdate, FieldDefinition);

		if (bMigratedSomething)
		{
			FieldMigrationRecord.Result = EMigrationResult::Success;
		}
		else
		{
			FieldMigrationRecord.ResultReason = TEXT("Field didn't migrate anything");
		}

		bWroteUpdate |= bMigratedSomething;
	}

	return bWroteUpdate;
}

EntityMigrator::EntityMigrator(const Entities& InEntities, const TSharedPtr<FJsonObject>& OldSchemaBundleJsonObject, const TSharedPtr<FJsonObject>& NewSchemaBundleJsonObject, SnapshotMigratorReport& OutMigrationReport) :
	MigrationReport(OutMigrationReport)
{
	OldSchemaDefinitions = MakeShareable<SchemaBundleDefinitions>(new SchemaBundleDefinitions(OldSchemaBundleJsonObject));
	NewSchemaDefinitions = MakeShareable<SchemaBundleDefinitions>(new SchemaBundleDefinitions(NewSchemaBundleJsonObject));
	ComponentMigratorPtr = MakeUnique<ComponentMigrator>(OldSchemaDefinitions, NewSchemaDefinitions);
	FilterAndGroupEntities(InEntities);
}

bool EntityMigrator::MigrateEntities(Entities& OutMigratedEntities)
{
	return MigrateSystemEntities(OutMigratedEntities) && MigrateStartupActorEntities(OutMigratedEntities) && MigrateDynamicActorEntities(OutMigratedEntities);
}

bool EntityMigrator::MigrateSystemEntities(Entities& OutMigratedEntities)
{
	for (auto& Entity : SystemEntities)
	{
		if (!MigrateSystemEntity(Entity, OutMigratedEntities))
		{
			return false;
		}
	}

	return true;
}

bool EntityMigrator::MigrateStartupActorEntities(Entities& OutMigratedEntities)
{
	for (auto& Entity : StartupActorEntities)
	{
		if (!MigrateStartupActorEntity(Entity, OutMigratedEntities))
		{
			return false;
		}
	}

	return true;
}

bool EntityMigrator::MigrateDynamicActorEntities(Entities& OutMigratedEntities)
{
	for (auto& Entity : DynamicActorEntities)
	{
		if (!MigrateDynamicActorEntity(Entity, OutMigratedEntities))
		{
			return false;
		}
	}

	return true;
}

bool EntityMigrator::MigrateSystemEntity(const Entity& SystemEntity, Entities& OutMigratedEntities)
{
	OutMigratedEntities.Add(SystemEntity);

	FEntityMigrationRecord& EntityMigrationRecord = MigrationReport.GetEntityMigrationRecord(SystemEntity.Key);
	EntityMigrationRecord.Result = EMigrationResult::Success;

	return true;
}

bool EntityMigrator::MigrateStartupActorEntity(const Entity& StartupActorEntity, Entities& OutMigratedEntities)
{
	// Todo: Make this actually startup-actor specific.
	// For now, just treat them like dynamic actors.
	return MigrateActorEntity(StartupActorEntity, true, OutMigratedEntities);
}

bool EntityMigrator::MigrateDynamicActorEntity(const Entity& DynamicActorEntity, Entities& OutMigratedEntities)
{
	return MigrateActorEntity(DynamicActorEntity, false, OutMigratedEntities);
}

bool EntityMigrator::MigrateActorEntity(const Entity& InEntity, bool bIsStartup, Entities& OutMigratedEntities)
{
	Components NewComponents;

	const Worker_EntityId EntityId = InEntity.Key;

	const Worker_ComponentData* UnrealMetadataComponentData = FindComponentById(InEntity.Value, SpatialConstants::UNREAL_METADATA_COMPONENT_ID);
	check(UnrealMetadataComponentData);

	FEntityMigrationRecord& EntityMigrationRecord = MigrationReport.GetEntityMigrationRecord(EntityId);
	// Since most branches below return failure, assume failed unless we reach the end of the migration, at which point we'll instead set success
	EntityMigrationRecord.Result = EMigrationResult::Failed;

	SpatialGDK::UnrealMetadata UnrealMetadata(*UnrealMetadataComponentData);

	UClass* ActorEntityClass = UnrealMetadata.GetNativeEntityClass();
	check(ActorEntityClass);
	check(ActorEntityClass->IsChildOf(AActor::StaticClass()));

	UWorld* World = GetAnyGameWorldServer();
	check(World);

	FActorSpawnParameters SpawnParams;
	SpawnParams.bTemporaryEditorActor = true;
	if (UnrealMetadata.StablyNamedRef.IsSet())
	{
		FUnrealObjectRef LevelRef = UnrealMetadata.StablyNamedRef->GetLevelReference();
		check(LevelRef.Path.IsSet());
		SpawnParams.OverrideLevel = FindLevelByName(World, LevelRef.Path.GetValue());
	}

	AActor* EntityActor = World->SpawnActor<AActor>(ActorEntityClass, SpawnParams);
	if (EntityActor == nullptr)
	{
		EntityMigrationRecord.ResultReason = FString::Printf(TEXT("Failed to spawn actor of class %s"), *GetNameSafe(ActorEntityClass));
		return false;
	}

	EntityActor->bNetStartup = bIsStartup;

	USpatialNetDriver* NetDriver = GetSpatialNetDriver();

	// Entity Id has no real semantic meaning outside of entities referencing other entities
	// Use a temporary Id here to avoid entities we're reading in from colliding with ones already spawned into the test world.
	const Worker_EntityId TempEntityId = NetDriver->PackageMap->AllocateEntityId();

	NetDriver->PackageMap->ResolveEntityActor(EntityActor, TempEntityId);

	USpatialActorChannel* Channel = Cast<USpatialActorChannel>(NetDriver->GetSpatialOSNetConnection()->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally));
	Channel->SetChannelActor(EntityActor, ESetChannelActorFlags::None);
	Channel->SetEntityId(TempEntityId);
	Channel->bCreatingNewEntity = true;

	ON_SCOPE_EXIT
	{
		EntityActor->bNetStartup = false;
		Channel->Close(EChannelCloseReason::Destroyed);
		NetDriver->RemoveActorChannel(TempEntityId, *Channel);
		World->DestroyActor(EntityActor);
		NetDriver->PackageMap->RemoveEntityActor(TempEntityId);
	};

	Channel->ReplicateActor();

	SpatialGDK::EntityFactory EntityFactory(NetDriver, NetDriver->PackageMap, NetDriver->ClassInfoManager, NetDriver->GetRPCService());
	uint32 BytesWritten;
	TArray<FWorkerComponentData> RawComponents = EntityFactory.CreateEntityComponents(Channel, BytesWritten);
	NewComponents.Append(RawComponents);

	// Adapted from SpatialSender::CreateLevelComponentData
	UWorld* ActorWorld = EntityActor->GetTypedOuter<UWorld>();
	if (ActorWorld != NetDriver->World)
	{
		const uint32 ComponentId = NetDriver->ClassInfoManager->GetComponentIdFromLevelPath(ActorWorld->GetOuter()->GetPathName());
		if (ComponentId != SpatialConstants::INVALID_COMPONENT_ID)
		{
			NewComponents.Add(SpatialGDK::ComponentFactory::CreateEmptyComponentData(ComponentId));
		}
		else
		{
			EntityMigrationRecord.ResultReason = FString::Printf(TEXT("Failed to find Streaming Level Component for Level %s, processing Actor %s. Have you generated schema?"), *ActorWorld->GetOuter()->GetPathName(), *EntityActor->GetPathName());
			return false;
		}
	}
	else
	{
		NewComponents.Add(SpatialGDK::ComponentFactory::CreateEmptyComponentData(SpatialConstants::NOT_STREAMED_COMPONENT_ID));
	}

	EntityMigrationRecord.ComponentMigrationRecords.Reserve(NewComponents.Num());

	for (Worker_ComponentData& NewComponent : NewComponents)
	{
		FComponentMigrationRecord& ComponentMigrationRecord = EntityMigrationRecord.ComponentMigrationRecords.AddDefaulted_GetRef();
		ComponentMigrationRecord.TargetComponentId = NewComponent.component_id;
		// Since most branches below skip, assume skipped unless we manage to migrate the component successfully,
		// at which point we'll instead set success
		ComponentMigrationRecord.Result = EMigrationResult::Skipped;

		Worker_ComponentId OldComponentId;
		if (!SchemaBundleDefinitions::GetCorrespondingComponentId(*NewSchemaDefinitions, *OldSchemaDefinitions, NewComponent.component_id, OldComponentId))
		{
			ComponentMigrationRecord.ResultReason = FString::Printf(TEXT("Component does not exist in old schema; no migration to be done"), NewComponent.component_id);
			continue;
		}

		ComponentMigrationRecord.SourceComponentId = OldComponentId;

		const Worker_ComponentData* OldComponent = FindComponentById(InEntity.Value, OldComponentId);
		if (OldComponent == nullptr)
		{
			ComponentMigrationRecord.ResultReason = FString::Printf(TEXT("Component does not exist on entity in the source snapshot; no migration to be done"), NewComponent.component_id, EntityId);
			continue;
		}

		if (!ComponentMigratorPtr->MigrateComponent(OldComponent, NewComponent, ComponentMigrationRecord))
		{
			ComponentMigrationRecord.Result = EMigrationResult::Failed;
			ComponentMigrationRecord.ResultReason = FString::Printf(TEXT("Failed to migrate component"), NewComponent.component_id);
			return false;
		}

		ComponentMigrationRecord.Result = EMigrationResult::Success;
	}

	EntityMigrationRecord.Result = EMigrationResult::Success;

	OutMigratedEntities.Add(EntityId, NewComponents);
	return true;
}

void EntityMigrator::FilterAndGroupEntities(const Entities& InEntities)
{
	TArray<FRegexPattern> ClasspathPatternRegexes;

	for (const FString& ClasspathPattern : GetDefault<USnapshotMigrationSettings>()->ClasspathAllowlistPatterns)
	{
		ClasspathPatternRegexes.Add(FRegexPattern(ClasspathPattern));
	}

	for (auto Entity : InEntities)
	{
		FEntityMigrationRecord& EntityMigrationRecord = MigrationReport.AddEntityMigrationRecord(Entity.Key);
		EntityMigrationRecord.SourceEntityId = Entity.Key;
		EntityMigrationRecord.Result = EMigrationResult::FailedFiltering;

		const Worker_ComponentData* UnrealMetadataComponentData = FindComponentById(Entity.Value, SpatialConstants::UNREAL_METADATA_COMPONENT_ID);

		if (UnrealMetadataComponentData == nullptr)
		{
			// System entity
			SystemEntities.Add(Entity);
			EntityMigrationRecord.Result = EMigrationResult::PassedFiltering;
			EntityMigrationRecord.Classification = EEntityClassification::System;
			continue;
		}

		SpatialGDK::UnrealMetadata UnrealMetadata(*UnrealMetadataComponentData);

		UClass* ActorClass = UnrealMetadata.GetNativeEntityClass();
		// No class could be loaded for this actor; it's possible it was renamed or deleted.
		// Todo: Handle this better in the future by providing userspace hooks to handle class migrations
		if (ActorClass == nullptr)
		{
			EntityMigrationRecord.ResultReason = FString::Printf(TEXT("Class %s failed to load"), *UnrealMetadata.ClassPath);
			continue;
		}
		// Actor was previously persistent but no longer is. Skip
		else if (ActorClass->HasAnySpatialClassFlags(SPATIALCLASS_NotPersistent))
		{
			EntityMigrationRecord.ResultReason = FString::Printf(TEXT("Class %s not persistent"), *GetNameSafe(ActorClass));
			continue;
		}

		if (!DoesClasspathPassFilter(ClasspathPatternRegexes, UnrealMetadata.ClassPath))
		{
			EntityMigrationRecord.ResultReason = FString::Printf(TEXT("Class %s filtered out"), *UnrealMetadata.ClassPath);
			continue;
		}

		const bool bIsStartup = UnrealMetadata.bNetStartup.IsSet() && UnrealMetadata.bNetStartup.GetValue();

		if (bIsStartup)
		{
			StartupActorEntities.Add(Entity);
			EntityMigrationRecord.Result = EMigrationResult::PassedFiltering;
			EntityMigrationRecord.Classification = EEntityClassification::Startup;
		}
		else
		{
			DynamicActorEntities.Add(Entity);
			EntityMigrationRecord.Result = EMigrationResult::PassedFiltering;
			EntityMigrationRecord.Classification = EEntityClassification::Dynamic;
		}
	}
}

bool EntityMigrator::DoesClasspathPassFilter(const TArray<FRegexPattern>& ClasspathPatternRegexes, const FString& Classpath)
{
	for (const FRegexPattern& Pattern : ClasspathPatternRegexes)
	{
		FRegexMatcher Matcher{ Pattern, Classpath };
		if (Matcher.FindNext())
		{
			return true;
		}
	}

	return false;
}

const Worker_ComponentData* EntityMigrator::FindComponentById(const Components& InComponents, const Worker_ComponentId Id)
{
	for (int i = 0; i < InComponents.Num(); ++i)
	{
		if (InComponents[i].component_id == Id)
		{
			return &InComponents[i];
		}
	}

	return nullptr;
}
