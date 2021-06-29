#include "PrometheusServer.h"

#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"

#include "Analytics.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Timespan.h"
#include "Stats/Stats.h"
#include "Stats/StatsData.h"

// Calculated from int64 UnixEpoch = FDateTime(1970, 1, 1).GetTicks();
constexpr int64 UnixEpochTicks = 621355968000000000;

static int64 UnixTimestampMS(const FDateTime& Time)
{
	return (Time.GetTicks() - UnixEpochTicks) / ETimespan::TicksPerMillisecond;
}

bool FPrometheusServer::Initialize()
{
	int32 PrometheusPort = -1;

	FParse::Value(FCommandLine::Get(), TEXT("prometheusPort="), PrometheusPort);

	if (PrometheusPort < 0)
	{
		UE_LOG(LogAnalytics, Log, TEXT("Prometheus metrics disabled."));
		return true;
	}

	Router = FHttpServerModule::Get().GetHttpRouter(PrometheusPort);

	TWeakPtr<FPrometheusServer> WeakThisPtr(AsShared());
	// Register a handler for /_worker_metrics
	// Runtime use "_metrics", just in case sometime worker & runtime in same pod.
	MetricsHandle = Router->BindRoute(FHttpPath("/_worker_metrics"), EHttpServerRequestVerbs::VERB_GET,
		[WeakThisPtr](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete) {
			auto SharedThis = WeakThisPtr.Pin();
			if (!SharedThis.IsValid())
			{
				return false;
			}
			return SharedThis->ProcessPrometheusRequest(Request, OnComplete);
		});

	if (!MetricsHandle.IsValid())
	{
		UE_LOG(LogAnalytics, Error,
			TEXT("FMetricsPrometheusEndpoint unable bind route: /_worker_metrics"));
		return false;
	}

	// Make sure our listener has started
	FHttpServerModule::Get().StartAllListeners();
	return true;
}

FPrometheusServer::~FPrometheusServer()
{
	if (Router)
	{
		Router->UnbindRoute(MetricsHandle);
		MetricsHandle.Reset();
	}
}

FString FPrometheusServer::Serialize()
{
	FString Buffer = TEXT("# UE4 worker metrics\n");
	FString LastName;
	for (const auto& Pair : Metrics)
	{
		// Spec wants a gap between unique metric names, so when our metric name changes, add a newline.
		if (LastName != Pair.Key)
		{
			Buffer += "\n";
		}

		for (const auto& LabelPair : *Pair.Value)
		{
			// Skip reporting any entries that have never been updated.
			if (!LabelPair.Value->HasBeenUpdated())
			{
				continue;
			}

			Buffer += Pair.Key + LabelPair.Key + TEXT(" ") + LabelPair.Value->ToString() + "\n";
		}

		LastName = Pair.Key;
	}

#if STATS
	if (const FGameThreadStatsData* ViewData = FLatestGameThreadStatsData::Get().Latest)
	{
		Buffer += "\n";

		const int64 Timestamp = UnixTimestampMS((FDateTime::UtcNow()));
		for (const auto& Pair : ViewData->NameToStatMap)
		{
			if (Pair.Value)
			{
				Buffer += FString::Printf(TEXT("stat{name=\"%s\",field=\"count\"} %u %lld\n"), *Pair.Key.ToString(), Pair.Value->GetValue_CallCount(EComplexStatField::IncAve), Timestamp);
				Buffer += FString::Printf(TEXT("stat{name=\"%s\",field=\"incave\"} %f %lld\n"), *Pair.Key.ToString(), FPlatformTime::ToMilliseconds64(Pair.Value->GetValue_Duration(EComplexStatField::IncAve)), Timestamp);
				Buffer += FString::Printf(TEXT("stat{name=\"%s\",field=\"incmax\"} %f %lld\n"), *Pair.Key.ToString(), FPlatformTime::ToMilliseconds64(Pair.Value->GetValue_Duration(EComplexStatField::IncMax)), Timestamp);
			}
		}
	}
#endif

	return Buffer;
}

bool FPrometheusServer::ProcessPrometheusRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	auto Response = FHttpServerResponse::Create(Serialize(), TEXT("text/plain; version=0.0.4"));

	OnComplete(MoveTemp(Response));

	return true;
}

TSharedRef<FPrometheusMetric> FPrometheusServer::GetMetric(const FString& Name, TArray<FPrometheusLabel> Labels)
{
	// If we don't have that metric at all, add it and return the first one.
	TSharedRef<TMap<const FString, TSharedRef<FPrometheusMetric>>> Entries = Metrics.FindOrAdd(Name, MakeShared<TMap<const FString, TSharedRef<FPrometheusMetric>>>());

	FString StrLabel(TEXT("{}"));

	if (Labels.Num() > 0)
	{
		// Sort our labels so if you specify one metric as {a=b,c=d}, and another as {c=d,a=b}, we consider them the same.
		Labels.Sort([](const FPrometheusLabel& A, const FPrometheusLabel& B) {
			return A.Key < B.Key;
		});

		StrLabel = TEXT("{");
		for (const FPrometheusLabel& Label : Labels)
		{
			StrLabel += (Label.Key + TEXT("=\"") + Label.Value + TEXT("\","));
		}
		// Chop off the last , from our labels above.
		// out = metric{label="value",label2="value"}
		StrLabel.LeftChopInline(1);
		StrLabel += TEXT("}");
	}

	return Entries->FindOrAdd(StrLabel, MakeShared<FPrometheusMetric>());
}

FString FPrometheusMetric::ToString() const
{
	return FString::Printf(TEXT("%f"), Value);
}

void FPrometheusMetric::Set(double InValue)
{
	Set(InValue, FDateTime::UtcNow());
}

void FPrometheusMetric::Set(double InValue, FDateTime InTimestamp)
{
	Value = InValue;
	Timestamp = UnixTimestampMS(InTimestamp);
}

void FPrometheusMetric::Increment(double Amount)
{
	Increment(Amount, FDateTime::UtcNow());
}

void FPrometheusMetric::Increment(double Amount, FDateTime InTimestamp)
{
	Value += Amount;
	Timestamp = UnixTimestampMS(InTimestamp);
}
