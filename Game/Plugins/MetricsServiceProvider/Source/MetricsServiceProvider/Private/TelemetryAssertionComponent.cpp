// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "TelemetryAssertionComponent.h"

#include "Algo/Transform.h"
#include "Analytics.h"
#include "JsonObjectConverter.h"
#include "MetricsServiceProvider.h"

FString FFailedTelemetryExpectation::ToString() const
{
	FString JsonString;
	FJsonObjectConverter::UStructToFormattedJsonObjectString<TCHAR, TPrettyJsonPrintPolicy>(FFailedTelemetryExpectation::StaticStruct(), this, JsonString, 0, 0);
	return JsonString;
}

// Sets default values for this component's properties
UTelemetryAssertionComponent::UTelemetryAssertionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

#if WITH_DEV_AUTOMATION_TESTS
void UTelemetryAssertionComponent::PrepareMetricsProviderForTest()
{
	OnTestCleanup = FSimpleDelegate::CreateWeakLambda(this, [PreTestEvents = MoveTemp(FAnalyticsProviderMetrics::MetricsProvider->EventPayloads)]() mutable {
		FAnalyticsProviderMetrics::MetricsProvider->EventPayloads = MoveTemp(PreTestEvents);
	});
}

void UTelemetryAssertionComponent::RestoreMetricsProviderFromTest()
{
	OnTestCleanup.ExecuteIfBound();
}
#endif

void UTelemetryAssertionComponent::AddExpectedTelemetryEvent(const EMetricsClass MetricsClass, const FString& EventName, const TArray<FMetricsEventAttr>& Attributes)
{
	ExpectedTelemetry.Add(FTelemetryEventDescriptor{ MetricsClass, EventName, Attributes });
}

#if WITH_DEV_AUTOMATION_TESTS
void UTelemetryAssertionComponent::Assert(TArray<FFailedTelemetryExpectation>& OutMissingTelemetry) const
{
	OutMissingTelemetry.Reset(ExpectedTelemetry.Num());

	TSharedPtr<FAnalyticsProviderMetrics> MetricsProvider = FAnalyticsProviderMetrics::MetricsProvider;

	// Warn if we have expected telemetry but 0 events were registered over the course of the run. This is suspicious.
	if (MetricsProvider->EventPayloads.Num() == 0 && ExpectedTelemetry.Num() > 0)
	{
		UE_LOG(LogAnalytics, Warning, TEXT("No telemetry events were fired over the course of the test run; this is highly suspicious."));
	}

	const auto DescriptorFromMetricsPayload = [](const FAnalyticsProviderMetrics::MetricsPayload& Payload) {
		FTelemetryEventDescriptor Descriptor;
		Descriptor.MetricsClass = (EMetricsClass) StaticEnum<EMetricsClass>()->GetValueByNameString(FString::Printf(TEXT("EMetricsClass::MetricsClass_%s"), *Payload.EventClass));
		Descriptor.EventName = Payload.EventType;
		for (const auto& Attribute : Payload.EventAttributes)
		{
			FMetricsEventAttr Attr;
			Attr.Name = Attribute.GetName();
			Attr.Value = Attribute.GetValue();
			Descriptor.Attributes.Add(Attr);
		}

		return Descriptor;
	};

	// Returned validator will return true if invoked with a payload that matches on class, name, and contains a superset of the attributes found in the descriptor.
	// This is defined as a lambda here because:
	//   - Nested structs can't be forward declared
	//   - MetricsPayload is not publicly accessible
	//   - The MetricsServiceProvider header is private and therefore inaccessible from this class's header
	auto GetValidator = [DescriptorFromMetricsPayload](const FTelemetryEventDescriptor& Descriptor, TArray<FTelemetryEventDescriptor>& OutMatchedOnClass, TArray<FTelemetryEventDescriptor>& OutMatchedOnName) {
		return [Descriptor, DescriptorFromMetricsPayload, &OutMatchedOnClass, &OutMatchedOnName](const FAnalyticsProviderMetrics::MetricsPayload& Payload) {
			FString MetricsClass = StaticEnum<EMetricsClass>()->GetValueAsString(Descriptor.MetricsClass);
			MetricsClass.RemoveFromStart("EMetricsClass::MetricsClass_");
			MetricsClass.ToLowerInline();

			if (MetricsClass != Payload.EventClass)
			{
				return false;
			}

			OutMatchedOnClass.Add(DescriptorFromMetricsPayload(Payload));

			if (Descriptor.EventName != Payload.EventType)
			{
				return false;
			}

			OutMatchedOnName.Add(DescriptorFromMetricsPayload(Payload));

			for (auto Attribute : Descriptor.Attributes)
			{
				auto AttributeMatchPredicate = [Attribute](const FAnalyticsEventAttribute& EventAttribute) {
					// If no value is provided on the attribute in the descriptor, we only check to see that the attribute exists.
					// If a value is provided, we check that the attribute exists AND has the expected value.
					return Attribute.Name == EventAttribute.GetName() && (Attribute.Value.IsEmpty() || Attribute.Value == EventAttribute.GetValue());
				};

				if (!Payload.EventAttributes.ContainsByPredicate(AttributeMatchPredicate))
				{
					return false;
				}
			}

			return true;
		};
	};

	for (auto Descriptor : ExpectedTelemetry)
	{
		TArray<FTelemetryEventDescriptor> MatchedOnClass;
		TArray<FTelemetryEventDescriptor> MatchedOnName;
		if (!MetricsProvider->EventPayloads.ContainsByPredicate(GetValidator(Descriptor, MatchedOnClass, MatchedOnName)))
		{
			FFailedTelemetryExpectation FailedExpectation;
			FailedExpectation.Expectation = Descriptor;
			if (MatchedOnName.Num() > 0)
			{
				FailedExpectation.ClosestEvents = MoveTemp(MatchedOnName);
			}
			else if (MatchedOnClass.Num() > 0)
			{
				FailedExpectation.ClosestEvents = MoveTemp(MatchedOnClass);
			}
			else
			{
				Algo::Transform(MetricsProvider->EventPayloads, FailedExpectation.ClosestEvents, DescriptorFromMetricsPayload);
			}

			OutMissingTelemetry.Add(FailedExpectation);
		}
	}
}
#endif
