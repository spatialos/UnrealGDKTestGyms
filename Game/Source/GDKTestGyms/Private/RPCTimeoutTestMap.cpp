// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutTestMap.h"

#include "RPCTimeoutGameMode.h"
#include "Engine/StreamableManager.h"
#include "EngineClasses/SpatialWorldSettings.h"

URPCTimeoutTestMap::URPCTimeoutTestMap()
	: UGeneratedTestMap(EMapCategory::CI_PREMERGE_SPATIAL_ONLY, TEXT("RPCTimeoutTestMap"))
{
	SetNumberOfClients(2);
}

void URPCTimeoutTestMap::CreateCustomContentForMap()
{
	ASpatialWorldSettings* WorldSettings = CastChecked<ASpatialWorldSettings>(World->GetWorldSettings());
	WorldSettings->DefaultGameMode = ARPCTimeoutGameMode::StaticClass();
}