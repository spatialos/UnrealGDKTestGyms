// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotMigrationLatentActions.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialPackageMapClient.h"
#include "Interop/Connection/SpatialConnectionManager.h"
#include "SnapshotMigratorModule.h"

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
}	 // namespace

bool FWaitForSpatialConnection::Update()
{
	UWorld* World = GetAnyGameWorldServer();
	if (World == nullptr)
	{
		return false;
	}

	USpatialGameInstance* GameInstance = World->GetGameInstance<USpatialGameInstance>();
	if (GameInstance == nullptr)
	{
		return false;
	}

	USpatialConnectionManager* ConnectionManager = GameInstance->GetSpatialConnectionManager();
	if (ConnectionManager == nullptr)
	{
		return false;
	}

	return ConnectionManager->GetWorkerConnection() != nullptr;
}

bool FWaitForNetDriver::Update()
{
	UWorld* World = GetAnyGameWorldServer();
	if (World == nullptr)
	{
		return false;
	}

	const USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver());
	if (NetDriver == nullptr)
	{
		return false;
	}

	return NetDriver->IsReady();
}

bool FMigrateSnapshot::Update()
{
	if (!SnapshotMigrationHelper::Migrate(SnapshotName))
	{
		UE_LOG(LogSnapshotMigration, Error, TEXT("Failed to complete the migration process!"));
	}

	return true;
}
