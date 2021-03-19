// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SnapshotMigratorModule.h"
#include "Tests/AutomationCommon.h"
#include "Tests/SnapshotMigrationHelper.h"
#include "Tests/SnapshotMigrationLatentActions.h"
#include "Tests/SnapshotMigrationSettings.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FSnapshotMigrationTest, "NWX.Snapshot Migration", EAutomationTestFlags::HighPriority | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

void FSnapshotMigrationTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const FString& MapPath : GetDefault<USnapshotMigrationSettings>()->Maps)
	{
		OutBeautifiedNames.Add(FPaths::GetBaseFilename(MapPath));
		OutTestCommands.Add(MapPath);
	}
}

bool FSnapshotMigrationTest::RunTest(const FString& Parameters)
{
	const FString MapPath = Parameters;
	const FString MapName = FPaths::GetBaseFilename(MapPath);

	if (!AutomationOpenMap(MapPath))
	{
		UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to open map %s!"), *MapPath);
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForSpecifiedMapToLoadCommand(MapName));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForSpatialConnection());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForNetDriver());
	ADD_LATENT_AUTOMATION_COMMAND(FMigrateSnapshot(MapName));

	return true;
}
