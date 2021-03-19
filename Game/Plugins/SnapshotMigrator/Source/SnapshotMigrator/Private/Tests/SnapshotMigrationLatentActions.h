// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Misc/AutomationTest.h"
#include "Tests/SnapshotMigrationHelper.h"

/** Begin Initialization / Setup Latent Commands */
DEFINE_LATENT_AUTOMATION_COMMAND(FWaitForSpatialConnection);
DEFINE_LATENT_AUTOMATION_COMMAND(FWaitForNetDriver);
/** End Initialization / Setup Latent Commands */

/** Begin Migration Latent Commands */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FMigrateSnapshot, const FString, SnapshotName);	  //-V801 -- We need to make a copy of the string here. Why? Not exactly sure, but we get funky characters if we don't.
/** End Migration Latent Commands */
