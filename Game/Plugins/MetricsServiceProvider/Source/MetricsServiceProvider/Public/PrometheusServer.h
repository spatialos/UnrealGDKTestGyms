#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"

/*
 * Basic Prometheus metrics exporter
 *
 * Expected Use:
 *   Call GetMetric() rarely, cache the result/you own it.  Each metric name/set of labels is a unique value.
 *   Call Set() as often as you want.
 *
 *   ProcessPrometheusRequest() will be called periodically by our metrics scraper.
 */

class IHttpRouter;

typedef TPair<FString, FString> FPrometheusLabel;

// Reference: https://github.com/prometheus/docs/blob/master/content/docs/instrumenting/exposition_formats.md#basic-info
class METRICSSERVICEPROVIDER_API FPrometheusMetric
{
public:
	FPrometheusMetric(){};

	FString ToString() const;

	// HasBeenUpdated is use to check to see if the metric has ever actually had values set.
	// We may end up with metrics that are created, but never updated and this is used to prune our results down a lot.
	bool HasBeenUpdated() const
	{
		return Timestamp != 0;
	};

	void Set(double Value);
	void Set(double Value, FDateTime Timestamp);

	void Increment(double Amount);
	void Increment(double Amount, FDateTime Timestamp);

private:
	double Value = 0.0;
	int64 Timestamp = 0;
};

class METRICSSERVICEPROVIDER_API FPrometheusServer : public TSharedFromThis<FPrometheusServer>
{
public:
	FPrometheusServer(){};
	virtual ~FPrometheusServer();
	bool Initialize();

	TSharedRef<FPrometheusMetric> GetMetric(const FString& Name, TArray<FPrometheusLabel> Labels);

private:
	FString Serialize();

	bool ProcessPrometheusRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

	TSharedPtr<IHttpRouter> Router = nullptr;
	FHttpRouteHandle MetricsHandle;

	TMap<const FString, TSharedRef<TMap<const FString, TSharedRef<FPrometheusMetric>>>> Metrics;
};
