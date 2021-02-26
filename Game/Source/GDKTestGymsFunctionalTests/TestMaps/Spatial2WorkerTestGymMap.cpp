// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Spatial2WorkerTestGymMap.h"
#include "SpatialGDK/Public/EngineClasses/SpatialWorldSettings.h"
#include "SpatialGDKFunctionalTests/Public/TestWorkerSettings.h"
#include "GDKTestGymsFunctionalTests/Tests/PredictedGameplayCuesTest/PredictedGameplayCuesTest.h"
#include "GDKTestGymsFunctionalTests/Tests/CrossServerAbilityActivationTest/CrossServerAbilityActivationTest.h"
#include "TestWorkerSettings.h"

USpatial2WorkerTestGymMap::USpatial2WorkerTestGymMap()
	: UGeneratedTestMap(EMapCategory::CI_PREMERGE_SPATIAL_ONLY, TEXT("Spatial2WorkerTestGymMap"))
{
}

void USpatial2WorkerTestGymMap::CreateCustomContentForMap()
{
	ULevel* CurrentLevel = World->GetCurrentLevel();

	AddActorToLevel<ACrossServerAbilityActivationTest>(CurrentLevel, FTransform::Identity);

	ASpatialWorldSettings* WorldSettings = CastChecked<ASpatialWorldSettings>(World->GetWorldSettings());
	WorldSettings->bEnableDebugInterface = true; // ACrossServerAbilityActivationTest requires the debug interface
	WorldSettings->SetMultiWorkerSettingsClass(UTest1x2FullInterestWorkerSettings::StaticClass());
}
