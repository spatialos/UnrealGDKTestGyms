// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialSingleWorkerTestGymMap.h"
#include "GDKTestGymsFunctionalTests/Tests/PredictedGameplayCuesTest/PredictedGameplayCuesTest.h"
#include "GDKTestGymsFunctionalTests/Tests/CrossServerAbilityActivationTest/CrossServerAbilityActivationTest.h"

USpatialSingleWorkerTestGymMap::USpatialSingleWorkerTestGymMap()
	: UGeneratedTestMap(EMapCategory::CI_PREMERGE, TEXT("SpatialSingleWorkerTestGymMap"))
{
}

void USpatialSingleWorkerTestGymMap::CreateCustomContentForMap()
{
	ULevel* CurrentLevel = World->GetCurrentLevel();
	AddActorToLevel<APredictedGameplayCuesTest>(CurrentLevel, FTransform::Identity);
}
