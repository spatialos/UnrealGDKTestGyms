// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MetricsBlueprintLibrary.h"

#include "TelemetryAssertionComponent.generated.h"

USTRUCT(BlueprintType)
struct METRICSSERVICEPROVIDER_API FTelemetryEventDescriptor
{
	GENERATED_BODY()

public:
	FTelemetryEventDescriptor() :
		FTelemetryEventDescriptor(EMetricsClass::MetricsClass_Default, TEXT(""), {})
	{
	}

	FTelemetryEventDescriptor(const EMetricsClass InMetricsClass, const FString& InEventName, const TArray<FMetricsEventAttr>& InAttributes) :
		MetricsClass(InMetricsClass), EventName(InEventName), Attributes(InAttributes)
	{
	}

	UPROPERTY(BlueprintReadOnly)
	EMetricsClass MetricsClass;

	UPROPERTY(BlueprintReadOnly)
	FString EventName;

	UPROPERTY(BlueprintReadOnly)
	TArray<FMetricsEventAttr> Attributes;
};

USTRUCT(BlueprintType)
struct METRICSSERVICEPROVIDER_API FFailedTelemetryExpectation
{
	GENERATED_BODY();

public:
	UPROPERTY(BlueprintReadOnly)
	FTelemetryEventDescriptor Expectation;

	UPROPERTY(BlueprintReadOnly)
	TArray<FTelemetryEventDescriptor> ClosestEvents;

	FString ToString() const;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class METRICSSERVICEPROVIDER_API UTelemetryAssertionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTelemetryAssertionComponent();

#if WITH_DEV_AUTOMATION_TESTS
	void PrepareMetricsProviderForTest();
	void RestoreMetricsProviderFromTest();
#endif

	/**
	 * Adds a telemetry event that must be fired over the course of the test.
	 * If no value is provided for a given attribute, we will only assert that an attribute with that name exists.
	 * If a value is provided for a given attribute, we will assert that an attribute with that name exists *and* that it has the provided value.
	 *
	 * @param MetricsClass	- The class of the telemetry event
	 * @param EventName		- The name of the telemetry event
	 * @param Attributes	- The set of attributes that must be present in a given event to satisfy this expectation.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Attributes"))
	void AddExpectedTelemetryEvent(const EMetricsClass MetricsClass, const FString& EventName, const TArray<FMetricsEventAttr>& Attributes);

#if WITH_DEV_AUTOMATION_TESTS
	void Assert(TArray<FFailedTelemetryExpectation>& OutMissingTelemetry) const;
#endif
	UPROPERTY(EditDefaultsOnly)
	TArray<FTelemetryEventDescriptor> ExpectedTelemetry;

private:
	FSimpleDelegate OnTestCleanup;
};
