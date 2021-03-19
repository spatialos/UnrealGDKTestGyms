// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SnapshotMigrationSettings.generated.h"

/**
 *
 */
UCLASS(Config = SnapshotMigrationSettings, DefaultConfig)
class USnapshotMigrationSettings : public UObject
{
	GENERATED_BODY()

public:
	USnapshotMigrationSettings();

	virtual void PostInitProperties() override;

	UPROPERTY()
	FString SourceSnapshotDirectory;

	UPROPERTY()
	FString MigratedSnapshotDirectory;

	UPROPERTY()
	FString SourceSchemaBundleFilepath;

	UPROPERTY()
	FString TargetSchemaBundleFilepath;

	UPROPERTY()
	TArray<FString> Maps;

	UPROPERTY(Config)
	TArray<FString> ClasspathAllowlistPatterns;

	UPROPERTY(Config)
	bool bOutputMigrationReportToLog = true;

private:
	void BuildDefaultFilepaths();
	void GetDefaultMaps();
	void CheckForCommandLineOverrides();

	// Default subdirs are rooted under the default spatial dir.
	const FString DEFAULT_SOURCE_SNAPSHOT_SUBDIR = "tmp/artifacts";
	const FString DEFAULT_MIGRATED_SNAPSHOT_SUBDIR = "snapshots";
	const FString DEFAULT_SOURCE_SCHEMA_SUBDIR = "tmp/artifacts";
	const FString DEFAULT_TARGET_SCHEMA_SUBDIR = "build/assembly/schema";
	const FString SCHEMA_BUNDLE_FILENAME = "schema.json";
};
