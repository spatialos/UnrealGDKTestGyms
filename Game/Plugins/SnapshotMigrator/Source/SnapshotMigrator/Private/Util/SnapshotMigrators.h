// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Util/SchemaBundleWrappers.h"
#include "Util/SnapshotHelperLibrary.h"
#include "Util/SnapshotMigratorReport.h"
#include "WorkerSDK/improbable/c_worker.h"

using Components = TArray<Worker_ComponentData>;
using Entity = TTuple<Worker_EntityId, Components>;
using Entities = TMap<Worker_EntityId, Components>;

class FieldMigrator
{
public:
	FieldMigrator(const TSharedPtr<SchemaBundleDefinitions>& InOldSchemaDefinitions, const TSharedPtr<SchemaBundleDefinitions>& InNewSchemaDefinitions);

	bool ShouldMigrateField(const Worker_ComponentData* OldComponent, const SchemaBundleFieldDefinition& OldFieldDefinition, Worker_ComponentUpdate& OutComponentUpdate, const SchemaBundleFieldDefinition& NewFieldDefinition);

private:
	TSharedPtr<SchemaBundleDefinitions> OldSchemaDefinitions;
	TSharedPtr<SchemaBundleDefinitions> NewSchemaDefinitions;

	// Todo: Roll SnapshotDataMigrator funcs into the FieldMigrator
	TUniquePtr<SnapshotDataMigrator> DataMigrator;
};

class ComponentMigrator
{
public:
	ComponentMigrator(const TSharedPtr<SchemaBundleDefinitions>& InOldSchemaDefinitions, const TSharedPtr<SchemaBundleDefinitions>& InNewSchemaDefinitions);

	bool MigrateComponent(const Worker_ComponentData* OldComponent, Worker_ComponentData& OutNewComponent, FComponentMigrationRecord& ComponentMigrationRecord);

private:
	bool ShouldUpdateComponent(const Worker_ComponentData* OldComponent, const Worker_ComponentId NewComponentId, Worker_ComponentUpdate& OutComponentUpdate, FComponentMigrationRecord& ComponentMigrationRecord);

	TUniquePtr<FieldMigrator> FieldMigratorPtr;

	TSharedPtr<SchemaBundleDefinitions> OldSchemaDefinitions;
	TSharedPtr<SchemaBundleDefinitions> NewSchemaDefinitions;
};

class EntityMigrator
{
public:
	EntityMigrator(const Entities& InEntities, const TSharedPtr<FJsonObject>& OldSchemaBundleJsonObject, const TSharedPtr<FJsonObject>& NewSchemaBundleJsonObject, SnapshotMigratorReport& OutMigrationResults);

	bool MigrateEntities(Entities& OutMigratedEntities);

private:
	bool MigrateSystemEntities(Entities& OutMigratedEntities);
	bool MigrateStartupActorEntities(Entities& OutMigratedEntities);
	bool MigrateDynamicActorEntities(Entities& OutMigratedEntities);

	bool MigrateSystemEntity(const Entity& SystemEntity, Entities& OutMigratedEntities);
	bool MigrateStartupActorEntity(const Entity& StartupActorEntity, Entities& OutMigratedEntities);
	bool MigrateDynamicActorEntity(const Entity& DynamicActorEntity, Entities& OutMigratedEntities);

	bool MigrateActorEntity(const Entity& InEntity, bool bIsStartup, Entities& OutMigratedEntities);

	void FilterAndGroupEntities(const Entities& InEntities);
	bool DoesClasspathPassFilter(const TArray<class FRegexPattern>& ClasspathPatternRegexes, const FString& Classpath);
	const Worker_ComponentData* FindComponentById(const Components& InComponents, const Worker_ComponentId Id);

	TSharedPtr<SchemaBundleDefinitions> OldSchemaDefinitions;
	TSharedPtr<SchemaBundleDefinitions> NewSchemaDefinitions;

	TUniquePtr<ComponentMigrator> ComponentMigratorPtr;

	Entities SystemEntities;
	Entities StartupActorEntities;
	Entities DynamicActorEntities;

	SnapshotMigratorReport& MigrationReport;
};
