#pragma once
#include "Analytics/Public/AnalyticsEventAttribute.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PrometheusServer.h"
#include "MetricsBlueprintLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPrometheusMetrics, Log, All);

/** Blueprint accessible version of the analytics event struct */
USTRUCT(BlueprintType)
struct FMetricsEventAttr
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analytics")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analytics")
	FString Value;

	FMetricsEventAttr()
	{
	}

	FMetricsEventAttr(FString InputName, FString InputValue) :
		Name(InputName),
		Value(InputValue)
	{
	}
};

UENUM(BlueprintType)
enum class EMetricsClass : uint8
{
	MetricsClass_Building,
	MetricsClass_Combat,
	MetricsClass_Crafting,
	MetricsClass_Gameplay,
	MetricsClass_Movement,
	MetricsClass_Prosperity,
	MetricsClass_Resources,
	MetricsClass_Interaction,
	MetricsClass_Objectives,

	MetricsClass_SimPlayers,

	MetricsClass_Default,
	MetricsClass_Session,

	MetricsClass_RPC,
	MetricsClass_Performance,

	MetricsClass_Login,			// Login flow related metrics
	MetricsClass_Connection,	// Connecting/traveling to deployments related metrics

	MetricsClass_EditorTelemetry,	 // Editor telemetry for internal stats
};

UCLASS(meta = (ScriptName = "MetricsLibrary"))
class METRICSSERVICEPROVIDER_API UMetricsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Records an event has happened for a specific class and specific player. This should be the preferred choice */
	UFUNCTION(BlueprintCallable, Category = "Telemetry", meta = (WorldContext = "WorldContextObject"))
	static void TelemetryEventForPlayer(const UObject* WorldContextObject, EMetricsClass EventClass, const FString& EventName, const APlayerState* PlayerState);

	/** Records an event has happened for a specific class and specific player. This should be the preferred choice */
	UFUNCTION(BlueprintCallable, Category = "Telemetry", meta = (WorldContext = "WorldContextObject"))
	static void TelemetryEventForPlayerWithAttributes(const UObject* WorldContextObject, EMetricsClass EventClass, const FString& EventName, const APlayerState* PlayerState, const TArray<FMetricsEventAttr>& Attributes);
	/// <summary>
	/// Internal player telemetry that allows overriding the gaurd that pervents sending client telemtry events.
	/// </summary>
	/// <param name="WorldContextObject">World Context</param>
	/// <param name="EventClass">Event Class</param>
	/// <param name="EventName">Event Name</param>
	/// <param name="PlayerState">Player state for retrieving id and location information</param>
	/// <param name="Attributes">Additional atttribute info</param>
	/// <param name="OverrideClientGuard"> client guard override, if false then telemtry on the client will be dropped</param>
	static void TelemetryEventForPlayerWithAttributesWithClientOverride(const UObject* WorldContextObject, EMetricsClass EventClass, const FString& EventName, const APlayerState* PlayerState, const TArray<FMetricsEventAttr>& Attributes, const bool OverrideClientGuard = false);

	/** Records an event has happened for a specific class */
	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	static void TelemetryEventWithAttributes(EMetricsClass EventClass, const FString& EventName, const TArray<FMetricsEventAttr>& Attributes);

	/** Records an event has happened for a specific class */
	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	static void TelemetryEvent(EMetricsClass EventClass, const FString& EventName);

	static void CacheProfileDetails(const FString& ProfileID, const FString& AccountID, const FString& ClientSessionID);

	static TSharedPtr<FPrometheusMetric> GetMetric(const FString& Name, TArray<FPrometheusLabel> Labels);
};
