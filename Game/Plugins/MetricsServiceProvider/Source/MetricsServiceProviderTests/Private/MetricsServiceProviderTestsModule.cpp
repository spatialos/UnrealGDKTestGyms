// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MetricsServiceProviderTests.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FMetricsServiceProviderTestsModule"

DEFINE_LOG_CATEGORY(LogMetricsServiceProviderTests);

void FMetricsServiceProviderTestsModule::StartupModule()
{
}

void FMetricsServiceProviderTestsModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetricsServiceProviderTestsModule, MetricsServiceProviderTests)
