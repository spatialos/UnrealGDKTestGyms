// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialSingleWorkerTestGymMap.h"
#include "GDKTestGymsFunctionalTests/Tests/PredictedGameplayCuesTest/PredictedGameplayCuesTest.h"
#include "GDKTestGymsFunctionalTests/Tests/CrossServerAbilityActivationTest/CrossServerAbilityActivationTest.h"

#include "Misc/EngineVersionComparison.h"

USpatialSingleWorkerTestGymMap::USpatialSingleWorkerTestGymMap()
	: UGeneratedTestMap(EMapCategory::CI_PREMERGE, TEXT("SpatialSingleWorkerTestGymMap"))
{
}

void USpatialSingleWorkerTestGymMap::CreateCustomContentForMap()
{
#if UE_VERSION_NEWER_THAN(4, 27, -1)
	ULevel* CurrentLevel = World->GetCurrentLevel();
	AddActorToLevel<APredictedGameplayCuesTest>(CurrentLevel, FTransform::Identity);
#endif
}
