// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotMigratorModule.h"
#include "SnapshotMigratorModuleInternal.h"

// Temporary; needed since we get link errors otherwise. Should be removed when this is moved into the GDK proper.
#include "Interop/SpatialClassInfoManager.h"
DEFINE_LOG_CATEGORY(LogSpatialClassInfoManager);

// Todo: Deprecate & remove
DEFINE_LOG_CATEGORY(LogSnapshotMigrator)
DEFINE_LOG_CATEGORY(LogSnapshotMigration)

void FSnapshotMigratorModule::StartupModule()
{
}

void FSnapshotMigratorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSnapshotMigratorModule, SnapshotMigrator)
