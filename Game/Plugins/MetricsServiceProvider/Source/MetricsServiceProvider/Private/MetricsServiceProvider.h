// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once
#include "Runtime/Online/HTTP/Public/Http.h"

#include "Containers/Ticker.h"
#include "HttpRetrySystem.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "PrometheusServer.h"

class METRICSSERVICEPROVIDER_API FAnalyticsProviderMetrics : public IAnalyticsProvider, public FTickerObjectBase
{
#if WITH_DEV_AUTOMATION_TESTS
	friend class FMetricsBlueprintLibrarySpec;
	friend class UTelemetryAssertionComponent;
#endif	  // WITH_DEV_AUTOMATION_TESTS
	friend class UMetricsBlueprintLibrary;

public:
	static TSharedPtr<IAnalyticsProvider> Create(const FString& Key, const FString& ApiEndpoint, const FString& BinaryApiKey, const FString& BinaryEndpointURL, const int32 BatchSizeThreshold, const int32 MaxAgeThreshold)
	{
		if (!Provider.IsValid())
		{
			MetricsProvider = TSharedPtr<FAnalyticsProviderMetrics>(new FAnalyticsProviderMetrics(Key, ApiEndpoint, BinaryApiKey, BinaryEndpointURL, BatchSizeThreshold, MaxAgeThreshold));
			Provider = TSharedPtr<IAnalyticsProvider>(MetricsProvider);
		}
		return Provider;
	}

	static void Destroy()
	{
		Provider.Reset();
		MetricsProvider.Reset();
	}

	virtual ~FAnalyticsProviderMetrics();

	//FTickerObjectBase
	bool Tick(float DeltaSeconds) override;

	virtual bool StartSession(const TArray<FAnalyticsEventAttribute>& Attributes) override;
	virtual void EndSession() override;
	virtual void FlushEvents() override;

	virtual void SetUserID(const FString& InUserID) override;
	virtual FString GetUserID() const override;

	virtual FString GetSessionID() const override;
	virtual bool SetSessionID(const FString& InSessionID) override;

	virtual void RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes) override;

	virtual void RecordItemPurchase(const FString& ItemId, const FString& Currency, int PerItemCost, int ItemQuantity) override;
	virtual void RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider) override;
	virtual void RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount) override;

	virtual void SetGender(const FString& InGender) override;
	virtual void SetLocation(const FString& InLocation) override;
	virtual void SetAge(const int32 InAge) override;

	virtual void RecordItemPurchase(const FString& ItemId, int ItemQuantity, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
	virtual void RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
	virtual void RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
	virtual void RecordError(const FString& Error, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
	virtual void RecordProgress(const FString& ProgressType, const FString& ProgressHierarchy, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;

	void SendBinaryFile(const FString& FullFile, const FString& FileName);
	void HandleResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) const;

	void TelemetryClassEvent(const FString& EventClass, const FString& EventName, const FString& EventProfileID, const TArray<FAnalyticsEventAttribute>& Attributes);

	void CacheProfileDetails(const FString& ProfileID, const FString& AccountID, const FString& ClientSessionID);

private:
	static const FString InvalidUserId;

	/** Eventually we will use API keys I assume */
	FString ApiKey = TEXT("API key Not Set");

	FString BinaryApiKey = TEXT("Binary API key not set");

	bool bHasSessionStarted = false;

	FString AccountId;
	FString ProfileId;
	FString WorkerId;
	FString EventSource;

	FString SessionId;

	/** Singleton for analytics */
	static TSharedPtr<IAnalyticsProvider> Provider;

	/** Singleton for our extra features */
	static TSharedPtr<FAnalyticsProviderMetrics> MetricsProvider;

	FAnalyticsProviderMetrics(const FString& Key, const FString& ApiEndpoint, const FString& BinaryApiKey, const FString& BinaryEndpointURL, const int32 BatchSizeThreshold, const float MaxAgeThreshold);

	FHttpModule* Http;

	FString EndPointURL = TEXT("API Endpoint Not set");

	FString BinaryEndpointURL = TEXT("Binary Endpoint URL not set");

	int32 EventId = 0;

	const int32 BatchSizeThreshold;

	FString GetStringifiedPayloads() const;

	const FString EngineVersion;

	const FString EventEnvironment;

	const float MaxAgeThresholdSeconds;

	float DeltaSecondsSinceFlush;

	// We report back a client session id and account id with telemetry, but want to keep this somewhat private.
	struct FProfileDetails
	{
		const FString AccountID;
		const FString ClientSessionID;
	};

	TMap<const FString, const FProfileDetails> CachedProfileDetails;

	TSharedPtr<class FHttpRetrySystem::FManager> HttpRetryManager;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateRequest();

	TSharedPtr<FPrometheusServer> Prometheus;
	TSharedPtr<FPrometheusMetric> TelemetryReported;

	/**
	 * Payload data based on schema defined here: https://docs.google.com/spreadsheets/d/1W_G1DxjpJW1aFGAr_kmPiSOVeo_ygb9GdhMhKf2HJW8/edit#gid=0
	 */
	struct MetricsPayload
	{
		/**
		 *  Event Environment e.g(development, staging, production)
		 */
		FString EventEnvironment = TEXT("Not set");
		/**
		 *  Increments by one after any event has been gathered. This will allow us to spot missing data.
		 */
		int32 EventIndex = 0;

		/**
		 * Denotes the source of the event, which can be either client or server side.
		 */
		FString EventSource = TEXT("Not set");

		/**
		 *  A higher order mnemonic classification of events (e.g. session).
		 */
		FString EventClass = TEXT("session");

		/**
		 *  A mnemonic event identifier (e.g. session_start).
		 */
		FString EventType = TEXT("Not set");

		/**
		 *  The ServerSessionID, which is unique per server instance
		 */
		FString ServerSessionID = TEXT("");

		/**
		 *  The ClientSessionID, which is unique per player session.
		 */
		FString ClientSessionID = TEXT("");

		/**
		 *   Version of the build
		 */
		FString EngineVersion = TEXT("TestGyms");

		/**
		 * Unix time the event occurred
		 */
		int32 EventTimestamp = 0;

		/**
		 * The AccountID of who reported the event(the real person)
		 */
		FString AccountID = TEXT("");

		/**
		 * The ProfileID of the player who reported the event(the character in game)
		 */
		FString ProfileID = TEXT("");

		/**
		 * The WorkerID the event was reported on.
		 */
		FString WorkerID = TEXT("");

		/**
		 *  Attribute data of the event
		 */
		TArray<FAnalyticsEventAttribute> EventAttributes;

		FString Stringify() const;

		MetricsPayload()
		{
			EventAttributes = TArray<FAnalyticsEventAttribute>();
		}
	};

	TArray<MetricsPayload> EventPayloads;

	void AugmentPayloadWithSessionDetails(MetricsPayload& PayloadData, const FString& EventProfileID);
};
