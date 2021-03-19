// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotMigrationSettings.h"

#include "Settings/ProjectPackagingSettings.h"
#include "SpatialGDKServicesConstants.h"

USnapshotMigrationSettings::USnapshotMigrationSettings()
{
}

void USnapshotMigrationSettings::PostInitProperties()
{
	Super::PostInitProperties();

	BuildDefaultFilepaths();
	GetDefaultMaps();
	CheckForCommandLineOverrides();
}

void USnapshotMigrationSettings::BuildDefaultFilepaths()
{
	const FString SpatialRootDir = SpatialGDKServicesConstants::SpatialOSDirectory;
	SourceSnapshotDirectory = FPaths::Combine(SpatialRootDir, DEFAULT_SOURCE_SNAPSHOT_SUBDIR);
	MigratedSnapshotDirectory = FPaths::Combine(SpatialRootDir, DEFAULT_MIGRATED_SNAPSHOT_SUBDIR);
	SourceSchemaBundleFilepath = FPaths::Combine(SpatialRootDir, DEFAULT_SOURCE_SCHEMA_SUBDIR, SCHEMA_BUNDLE_FILENAME);
	TargetSchemaBundleFilepath = FPaths::Combine(SpatialRootDir, DEFAULT_TARGET_SCHEMA_SUBDIR, SCHEMA_BUNDLE_FILENAME);
}

void USnapshotMigrationSettings::GetDefaultMaps()
{
	UProjectPackagingSettings* PackagingSettings = CastChecked<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
	for (const auto& MapToCook : PackagingSettings->MapsToCook)
	{
		const FString MapPath = MapToCook.FilePath;
		// Don't automatically try to migrate temporal realms (at least for now)
		if (!MapPath.Contains("Temporal"))
		{
			Maps.Add(MapPath);
		}
	}
}

void USnapshotMigrationSettings::CheckForCommandLineOverrides()
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	FCommandLine::Parse(FCommandLine::Get(), Tokens, Switches);

	for (const FString& CLSwitch : Switches)
	{
		const TCHAR* TempCmd = *CLSwitch;
		// Need to skip first character as it'll be '='
		auto GetSwitchValue = [](const TCHAR* Switch) { return FString{ Switch }.RightChop(1); };
		if (FParse::Command(&TempCmd, TEXT("Maps")))
		{
			Maps.Empty();
			GetSwitchValue(TempCmd).ParseIntoArray(Maps, TEXT(";"));
		}
		else if (FParse::Command(&TempCmd, TEXT("SourceSnapshotDirectory")))
		{
			SourceSnapshotDirectory = GetSwitchValue(TempCmd);
		}
		else if (FParse::Command(&TempCmd, TEXT("MigratedSnapshotDirectory")))
		{
			MigratedSnapshotDirectory = GetSwitchValue(TempCmd);
		}
		else if (FParse::Command(&TempCmd, TEXT("SourceSchemaBundle")))
		{
			SourceSchemaBundleFilepath = GetSwitchValue(TempCmd);
		}
		else if (FParse::Command(&TempCmd, TEXT("TargetSchemaBundle")))
		{
			TargetSchemaBundleFilepath = GetSwitchValue(TempCmd);
		}
	}
}
