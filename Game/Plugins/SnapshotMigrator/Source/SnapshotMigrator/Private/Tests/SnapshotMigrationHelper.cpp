// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotMigrationHelper.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SnapshotMigratorModule.h"
#include "Tests/SnapshotMigrationSettings.h"
#include "Util/SnapshotMigrators.h"
#include "Util/SnapshotStreamHelpers.h"

namespace
{
bool LoadJsonSchemaBundle(const FString& BundleFilepath, TSharedPtr<FJsonObject>& OutJsonObject)
{
	FString SchemaBundleJson;
	if (!FFileHelper::LoadFileToString(SchemaBundleJson, *BundleFilepath))
	{
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SchemaBundleJson);
	return FJsonSerializer::Deserialize(Reader, OutJsonObject) && OutJsonObject.IsValid();
}
}	 // namespace

bool SnapshotMigrationHelper::Migrate(const FString& SnapshotName)
{
	const USnapshotMigrationSettings* SnapshotMigrationSettings = GetDefault<USnapshotMigrationSettings>();

	TSharedPtr<FJsonObject> OldSchemaBundleJsonObject;
	TSharedPtr<FJsonObject> NewSchemaBundleJsonObject;

	const bool bLoadedOld = LoadJsonSchemaBundle(SnapshotMigrationSettings->SourceSchemaBundleFilepath, OldSchemaBundleJsonObject);
	const bool bLoadedNew = LoadJsonSchemaBundle(SnapshotMigrationSettings->TargetSchemaBundleFilepath, NewSchemaBundleJsonObject);

	if (bLoadedOld)
	{
		UE_LOG(LogSnapshotMigration, Log, TEXT("Loaded source schema bundle %s"), *SnapshotMigrationSettings->SourceSchemaBundleFilepath);
	}
	else
	{
		UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to load source schema bundle %s"), *SnapshotMigrationSettings->SourceSchemaBundleFilepath);
	}

	if (bLoadedNew)
	{
		UE_LOG(LogSnapshotMigration, Log, TEXT("Loaded target schema bundle %s"), *SnapshotMigrationSettings->TargetSchemaBundleFilepath);
	}
	else
	{
		UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to load target schema bundle %s"), *SnapshotMigrationSettings->TargetSchemaBundleFilepath);
	}

	if (!(bLoadedOld && bLoadedNew))
	{
		return false;
	}

	const FString OldSnapshotFilepath = FPaths::Combine(SnapshotMigrationSettings->SourceSnapshotDirectory, FString::Printf(TEXT("%s.snapshot"), *SnapshotName));
	const FString NewTempSnapshotFilepath = FPaths::Combine(SnapshotMigrationSettings->MigratedSnapshotDirectory, FString::Printf(TEXT("%s.snapshot.tmp"), *SnapshotName));
	const FString NewFinalSnapshotFilepath = FPaths::Combine(SnapshotMigrationSettings->MigratedSnapshotDirectory, FString::Printf(TEXT("%s.snapshot"), *SnapshotName));

	// Scoped to allow Input/OutputSnapshotHelper to release stream resources on scope exit
	{
		InputSnapshotHelper InputSnapshot(OldSnapshotFilepath);
		const bool bInitializedInputStream = InputSnapshot.IsStreamValid();
		if (!bInitializedInputStream)
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to initialize input snapshot stream from %s"), *OldSnapshotFilepath);
			return false;
		}
		else
		{
			UE_LOG(LogSnapshotMigration, Log, TEXT("Initialized input snapshot stream from %s"), *OldSnapshotFilepath);
		}

		OutputSnapshotHelper OutputSnapshot(NewTempSnapshotFilepath);
		const bool bInitializedOutputStream = OutputSnapshot.IsStreamValid();
		if (!bInitializedOutputStream)
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to initialize output snapshot stream to %s"), *NewTempSnapshotFilepath);
			return false;
		}
		else
		{
			UE_LOG(LogSnapshotMigration, Log, TEXT("Initialized output snapshot stream to %s"), *NewTempSnapshotFilepath);
		}

		Entities EntitiesFromSnapshot;
		if (!InputSnapshot.ReadEntities(EntitiesFromSnapshot))
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to read entities from snapshot"));
			return false;
		}

		SnapshotMigratorReport MigrationResults(EntitiesFromSnapshot.Num());
		EntityMigrator Migrator(EntitiesFromSnapshot, OldSchemaBundleJsonObject, NewSchemaBundleJsonObject, MigrationResults);

		Entities EntitiesToWriteToSnapshot;
		if (!Migrator.MigrateEntities(EntitiesToWriteToSnapshot))
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to migrate one or more entities"));
			return false;
		}

		if (SnapshotMigrationSettings->bOutputMigrationReportToLog)
		{
			MigrationResults.OutputReportToLog();
		}

		if (!OutputSnapshot.WriteEntitiesToSnapshot(EntitiesToWriteToSnapshot))
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to write entities to snapshot"));
			return false;
		}
		else
		{
			UE_LOG(LogSnapshotMigration, Log, TEXT("Succeeded writing snapshot entities!"));
		}
	}

	return IFileManager::Get().Move(*NewFinalSnapshotFilepath, *NewTempSnapshotFilepath, true, true);
}
