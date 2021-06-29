// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "MetricsServiceProviderEditor.h"

#include "Analytics.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "MetricsServiceProviderSettings.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FMetricsEditorModule, MetricsServiceProviderEditor)
#define LOCTEXT_NAMESPACE "Metrics"

void FMetricsEditorModule::StartupModule()
{
}

void FMetricsEditorModule::ShutdownModule()
{
}

UMetricsServiceProviderSettings::UMetricsServiceProviderSettings(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	SettingsDisplayName = LOCTEXT("SettingsDisplayName", "Metrics Service Provider");
	SettingsTooltip = LOCTEXT("SettingsTooltip", "Metrics analytics configuration settings");
}

void UMetricsServiceProviderSettings::ReadConfigSettings()
{
	// LOAD API End point
	FString ReadApiEndpoint = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetReleaseIniSection(), TEXT("ApiEndpoint"), true);
	ReleaseApiEndpoint = ReadApiEndpoint;

	ReadApiEndpoint = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetDebugIniSection(), TEXT("ApiEndpoint"), true);
	if (ReadApiEndpoint.Len() == 0)
	{
		ReadApiEndpoint = ReleaseApiEndpoint;
	}
	DebugApiEndpoint = ReadApiEndpoint;

	ReadApiEndpoint = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetTestIniSection(), TEXT("ApiEndpoint"), true);
	if (ReadApiEndpoint.Len() == 0)
	{
		ReadApiEndpoint = ReleaseApiEndpoint;
	}
	TestApiEndpoint = ReadApiEndpoint;

	ReadApiEndpoint = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetDevelopmentIniSection(), TEXT("ApiEndpoint"), true);
	if (ReadApiEndpoint.Len() == 0)
	{
		ReadApiEndpoint = ReleaseApiEndpoint;
	}
	DevelopmentApiEndpoint = ReadApiEndpoint;

	// LOAD API KEYS
	FString ReadApiKey = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetReleaseIniSection(), TEXT("MetricsApiKey"), true);
	ReleaseApiKey = ReadApiKey;

	ReadApiKey = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetDebugIniSection(), TEXT("MetricsApiKey"), true);
	if (ReadApiKey.Len() == 0)
	{
		ReadApiKey = ReleaseApiKey;
	}
	DebugApiKey = ReadApiKey;

	ReadApiKey = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetTestIniSection(), TEXT("MetricsApiKey"), true);
	if (ReadApiKey.Len() == 0)
	{
		ReadApiKey = ReleaseApiKey;
	}
	TestApiKey = ReadApiKey;

	ReadApiKey = FAnalytics::Get().GetConfigValueFromIni(GetIniName(), GetDevelopmentIniSection(), TEXT("MetricsApiKey"), true);
	if (ReadApiKey.Len() == 0)
	{
		ReadApiKey = ReleaseApiKey;
	}
	DevelopmentApiKey = ReadApiKey;
}

void UMetricsServiceProviderSettings::WriteConfigSettings()
{
	FAnalytics::Get().WriteConfigValueToIni(GetIniName(), GetReleaseIniSection(), TEXT("MetricsApiKey"), ReleaseApiKey);
	FAnalytics::Get().WriteConfigValueToIni(GetIniName(), GetTestIniSection(), TEXT("MetricsApiKey"), TestApiKey);
	FAnalytics::Get().WriteConfigValueToIni(GetIniName(), GetDebugIniSection(), TEXT("MetricsApiKey"), DebugApiKey);
	FAnalytics::Get().WriteConfigValueToIni(GetIniName(), GetDevelopmentIniSection(), TEXT("MetricsApiKey"), DevelopmentApiKey);
}

#undef LOCTEXT_NAMESPACE
