// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutTestMap.h"

#include "MaterialArray_CPP.h"
#include "RPCTimeoutGameMode.h"
#include "Engine/StaticMeshActor.h"
#include "EngineClasses/SpatialWorldSettings.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerStart.h"

URPCTimeoutTestMap::URPCTimeoutTestMap()
	: UGeneratedTestMap(EMapCategory::CI_PREMERGE_SPATIAL_ONLY, TEXT("RPCTimeoutTestMap"))
{
	SetNumberOfClients(2);
	

}

void URPCTimeoutTestMap::CreateCustomContentForMap()
{
	ASpatialWorldSettings* WorldSettings = CastChecked<ASpatialWorldSettings>(World->GetWorldSettings());
	WorldSettings->DefaultGameMode = ARPCTimeoutGameMode::StaticClass();
	
	ULevel* CurrentLevel = World->GetCurrentLevel();
	AddActorToLevel<AMaterialArray_CPP>(CurrentLevel, FTransform::Identity);
}