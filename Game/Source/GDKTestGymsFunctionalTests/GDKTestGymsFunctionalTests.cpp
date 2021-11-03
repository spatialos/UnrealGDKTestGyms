// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsFunctionalTests.h"
#include "LogGymsSpatialFunctionalTest.h"
#include "TestMapGeneration.h"

DEFINE_LOG_CATEGORY(LogGymsSpatialFunctionalTest);

IMPLEMENT_GAME_MODULE(FGDKTestGymsFunctionalTestsModule, GDKTestGymsFunctionalTests)

#define LOCTEXT_NAMESPACE "FGDKTestGymsFunctionalTestsModule"

void FGDKTestGymsFunctionalTestsModule::StartupModule()
{
	// Ensure that when running with multiple-processes, only the Editor process generates the test maps
	if (GIsEditor)
	{
		FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([]() {
			SpatialGDK::TestMapGeneration::GenerateTestMaps();
			});
	}
}


#undef LOCTEXT_NAMESPACE
