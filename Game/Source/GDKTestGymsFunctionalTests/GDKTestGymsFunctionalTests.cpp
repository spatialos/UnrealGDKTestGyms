// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsFunctionalTests.h"
#include "LogGymsSpatialFunctionalTest.h"
#include "TestMapGeneration.h"

DEFINE_LOG_CATEGORY(LogGymsSpatialFunctionalTest);

IMPLEMENT_GAME_MODULE(FGDKTestGymsFunctionalTestsModule, GDKTestGymsFunctionalTests)

#define LOCTEXT_NAMESPACE "FGDKTestGymsFunctionalTestsModule"

void FGDKTestGymsFunctionalTestsModule::StartupModule()
{
   FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([]() {
	   SpatialGDK::TestMapGeneration::GenerateTestMaps();
   });
}

void FGDKTestGymsFunctionalTestsModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE