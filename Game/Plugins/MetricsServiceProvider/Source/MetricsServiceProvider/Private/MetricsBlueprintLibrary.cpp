#include "MetricsBlueprintLibrary.h"

#include "Analytics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
#include "MetricsServiceProvider.h"

#define LOCTEXT_NAMESPACE "K2Node"
DEFINE_LOG_CATEGORY(LogPrometheusMetrics);

/**
 * Converts the UObject accessible array into the native only analytics array type
 */
static inline TArray<FAnalyticsEventAttribute> ConvertAttrs(const TArray<FMetricsEventAttr>& Attributes)
{
	TArray<FAnalyticsEventAttribute> Converted;
	Converted.Reserve(Attributes.Num());
	for (const FMetricsEventAttr& Attr : Attributes)
	{
		Converted.Emplace(Attr.Name, Attr.Value);
	}
	return Converted;
}

static inline FString EnumToString(EMetricsClass EventClass)
{
	FString Name = StaticEnum<EMetricsClass>()->GetValueAsString(EventClass);
	Name.RemoveFromStart("EMetricsClass::MetricsClass_");
	Name.ToLowerInline();

	return Name;
}

void UMetricsBlueprintLibrary::TelemetryEventForPlayer(const UObject* WorldContextObject, EMetricsClass EventClass, const FString& EventName, const APlayerState* PlayerState)
{
	TelemetryEventForPlayerWithAttributesWithClientOverride(WorldContextObject, EventClass, EventName, PlayerState, TArray<FMetricsEventAttr>(), false);
}

void UMetricsBlueprintLibrary::TelemetryEventForPlayerWithAttributesWithClientOverride(const UObject* WorldContextObject, EMetricsClass EventClass, const FString& EventName, const APlayerState* PlayerState, const TArray<FMetricsEventAttr>& Attributes, const bool OverrideClientGuard /*= false*/)
{
	if (!FAnalyticsProviderMetrics::MetricsProvider.IsValid())
	{
		UE_LOG(LogPrometheusMetrics, Warning, TEXT("RecordEventForPlayer: MetricsProvider is not a valid pointer"));
		return;
	}

	FString MetricClass = EnumToString(EventClass);
	const UWorld* World = WorldContextObject->GetWorld();

	// Only report metrics when we are on the server(and have a playerstate) or if this is a native call with a by pass
	if (PlayerState != nullptr && World != nullptr && (OverrideClientGuard || World->GetNetMode() != NM_Client) && PlayerState->GetUniqueId().IsValid())
	{
		TArray<FAnalyticsEventAttribute> AnalyticsAttributes = ConvertAttrs(Attributes);
		AnalyticsAttributes.Add(FAnalyticsEventAttribute("Map", World->GetMapName()));

		const APawn* Pawn = PlayerState->GetPawn();
		if (Pawn != nullptr)
		{
			const FVector& Pos = Pawn->GetActorLocation();
			const FString Position = FString::Printf(TEXT("%s, %s, %s"), Pos.X, Pos.Y, Pos.Z);
			AnalyticsAttributes.Add(FAnalyticsEventAttribute("Position", Position));
		}

		FAnalyticsProviderMetrics::MetricsProvider->TelemetryClassEvent(MetricClass, EventName, PlayerState->GetUniqueId()->ToString(), AnalyticsAttributes);
	}
	else
	{
		UE_LOG(LogPrometheusMetrics, Log, TEXT("Attempting to record event for player as client or NULL playerstate, or no world: %s: %s"), *MetricClass, *EventName);
	}
}
void UMetricsBlueprintLibrary::TelemetryEventForPlayerWithAttributes(const UObject* WorldContextObject, EMetricsClass EventClass, const FString& EventName, const APlayerState* PlayerState, const TArray<FMetricsEventAttr>& Attributes)
{
	TelemetryEventForPlayerWithAttributesWithClientOverride(WorldContextObject, EventClass, EventName, PlayerState, Attributes, false);
}

void UMetricsBlueprintLibrary::TelemetryEvent(EMetricsClass EventClass, const FString& EventName)
{
	UMetricsBlueprintLibrary::TelemetryEventWithAttributes(EventClass, EventName, TArray<FMetricsEventAttr>());
}

void UMetricsBlueprintLibrary::TelemetryEventWithAttributes(EMetricsClass EventClass, const FString& EventName, const TArray<FMetricsEventAttr>& Attributes)
{
	if (FAnalyticsProviderMetrics::MetricsProvider.IsValid())
	{
		FString MetricClass = EnumToString(EventClass);

		FAnalyticsProviderMetrics::MetricsProvider->TelemetryClassEvent(MetricClass, EventName, "", ConvertAttrs(Attributes));
	}
	else
	{
		UE_LOG(LogPrometheusMetrics, Warning, TEXT("RecordEvent: MetricsProvider is not a valid pointer"));
	}
}

TSharedPtr<FPrometheusMetric> UMetricsBlueprintLibrary::GetMetric(const FString& Name, const TArray<FPrometheusLabel>& Labels)
{
	if (FAnalyticsProviderMetrics::MetricsProvider.IsValid())
	{
		return FAnalyticsProviderMetrics::MetricsProvider->Prometheus->GetMetric(Name, Labels);
	}
	return nullptr;
}

void UMetricsBlueprintLibrary::CacheProfileDetails(const FString& ProfileID, const FString& AccountID, const FString& ClientSessionID)
{
	if (FAnalyticsProviderMetrics::MetricsProvider.IsValid())
	{
		return FAnalyticsProviderMetrics::MetricsProvider->CacheProfileDetails(ProfileID, AccountID, ClientSessionID);
	}
}

#undef LOCTEXT_NAMESPACE
