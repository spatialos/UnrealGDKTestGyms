#include "MetricsBlueprintLibrary.h"
#include "MetricsServiceProviderTests.h"
#include "Misc/AutomationTest.h"

#include "MetricsServiceProvider/Private/MetricsServiceProvider.h"

#define METRICS_TEST_PREFIX "Project.GDKTestGyms Cpp.Metrics."

BEGIN_DEFINE_SPEC(FMetricsBlueprintLibrarySpec, METRICS_TEST_PREFIX "MetricsBlueprintLibrary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

END_DEFINE_SPEC(FMetricsBlueprintLibrarySpec)

void FMetricsBlueprintLibrarySpec::Define()
{
	Describe("FMetricsBlueprintLibrary", [this]() {
		Describe("Adds telemetry event", [this]() {
			It("Converts event class enum", [this]() {
				struct ClassToEvent
				{
					EMetricsClass Class;
					FString Str;
				};
				// Doesn't have to be an exhaustive list, but just testing a sampling
				ClassToEvent Tests[] = {
					{ EMetricsClass::MetricsClass_Building, TEXT("building") },
					{ EMetricsClass::MetricsClass_Combat, TEXT("combat") },
					{ EMetricsClass::MetricsClass_Connection, TEXT("connection") },
					{ EMetricsClass::MetricsClass_Default, TEXT("default") },
					{ EMetricsClass::MetricsClass_Login, TEXT("login") },

				};
				for (const ClassToEvent Test : Tests)
				{
					UMetricsBlueprintLibrary::TelemetryEvent(Test.Class, TEXT("testevent"));
					TSharedPtr<FAnalyticsProviderMetrics> Metrics = FAnalyticsProviderMetrics::MetricsProvider;
					const FString& Payload = Metrics->GetStringifiedPayloads();

					TestTrue(FString::Printf(TEXT("contains event of class: %s"), *Test.Str), Payload.Contains(FString::Printf(TEXT("eventClass\":\"%s\""), *Test.Str)));
					Metrics->FlushEvents();
				}
			});
		});
	});
}
