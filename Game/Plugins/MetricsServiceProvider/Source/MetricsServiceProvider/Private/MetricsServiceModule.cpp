// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#include "MetricsServiceModule.h"

#include "Analytics.h"
#include "Engine/World.h"
#include "HttpRetrySystem.h"
#include "MetricsBlueprintLibrary.h"
#include "MetricsServiceProvider.h"
#include "Misc/App.h"
#include "Misc/EngineBuildSettings.h"
#include "PrometheusServer.h"
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformProcess.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/DateTime.h"
#include "Runtime/Core/Public/Misc/EngineVersion.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_MODULE(FAnalyticsMetricsServiceModule, MetricsServiceProvider)

TSharedPtr<IAnalyticsProvider> FAnalyticsProviderMetrics::Provider;
TSharedPtr<FAnalyticsProviderMetrics> FAnalyticsProviderMetrics::MetricsProvider;

const FString GEditorTelemetryPrefix = "MetricsEditorTelemetry_";
FString ConvertToCamelCase(FString& Input)
{
	if (Input.Len() == 0)
	{
		return Input;
	}

	Input[0] = Input.Left(1).ToLower().GetCharArray()[0];
	return Input;
}

static FString GetEventSource()
{
#if WITH_EDITOR
	return "editor";
#else
	if (GWorld)
	{
		switch (GWorld->GetNetMode())
		{
			case NM_Client:
				return "client";
			case NM_Standalone:
				return "standalone";
			default:
				return "server";
		}
	}
	else
	{
		// If we don't have a world, we are neither client or server
		// so standalone makes the most sense(and can be the case on client shutdown)
		return "standalone";
	}
#endif
}

void FAnalyticsMetricsServiceModule::StartupModule()
{
}

void FAnalyticsMetricsServiceModule::ShutdownModule()
{
	FAnalyticsProviderMetrics::Destroy();
}

TSharedPtr<IAnalyticsProvider> FAnalyticsMetricsServiceModule::CreateAnalyticsProvider(const FAnalyticsProviderConfigurationDelegate& GetConfigValue) const
{
	// Use our project name for our analytics endpoints.  By default UE4 uses the compiled build type of the game.
	FString ProjectName;
	FString AnalyticsSection;
	if (FParse::Value(FCommandLine::Get(), TEXT("projectName"), ProjectName))
	{
		AnalyticsSection = TEXT("Analytics_") + ProjectName;
	}
	else
	{
		AnalyticsSection = TEXT("Analytics");
	}

	const FString Key = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, AnalyticsSection, TEXT("ApiKey"), true);
	const FString ApiEndpoint = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, AnalyticsSection, TEXT("ApiEndpoint"), true);
	const int32 BatchSize = FCString::Atoi(*FAnalytics::Get().GetConfigValueFromIni(GEngineIni, AnalyticsSection, TEXT("BatchSize"), true));
	const FString BinaryKey = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, AnalyticsSection, TEXT("BinaryApiKey"), true);
	const FString BinaryEndpoint = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, AnalyticsSection, TEXT("BinaryEndpoint"), true);
	const float MaxAge = FCString::Atof(*FAnalytics::Get().GetConfigValueFromIni(GEngineIni, AnalyticsSection, TEXT("MaxAge"), true));

	return FAnalyticsProviderMetrics::Create(Key, ApiEndpoint, BinaryKey, BinaryEndpoint, BatchSize, MaxAge);
}

// Provider
const FString FAnalyticsProviderMetrics::InvalidUserId = TEXT("User Not Set");

// The maximum number of retries we will attempt.
const uint32 RetryLimitCount = 10;
// The maximum number of seconds we will try to retry inside of.
const uint32 RetryLimitTime = 60;

FAnalyticsProviderMetrics::FAnalyticsProviderMetrics(const FString& Key, const FString& ApiEndpoint, const FString& BinaryKey, const FString& BinaryEndpoint, const int32 BatchSize, const float MaxAge) :
	ApiKey(Key),
	BinaryApiKey(BinaryKey),
	EndPointURL(ApiEndpoint),
	BinaryEndpointURL(BinaryEndpoint),
	BatchSizeThreshold(BatchSize),
	EngineVersion(*FEngineVersion::Current().ToString()),
	EventEnvironment(LexToString(FApp::GetBuildConfiguration())),
	MaxAgeThresholdSeconds(MaxAge)
{
	HttpRetryManager = MakeShared<FHttpRetrySystem::FManager>(
		FHttpRetrySystem::FRetryLimitCountSetting(),
		FHttpRetrySystem::FRetryTimeoutRelativeSecondsSetting());

	Http = &FHttpModule::Get();

	Prometheus = MakeShared<FPrometheusServer>();
	Prometheus->Initialize();

	DeltaSecondsSinceFlush = 0;

	TelemetryReported = Prometheus->GetMetric(TEXT("telemetry_events"), TArray<FPrometheusLabel>{});

	TelemetryClassEvent(TEXT("session"), TEXT("Metrics Started"), ProfileId, TArray<FAnalyticsEventAttribute>());
}

FAnalyticsProviderMetrics::~FAnalyticsProviderMetrics()
{
	if (bHasSessionStarted)
	{
		EndSession();
	}
	TelemetryClassEvent(TEXT("session"), TEXT("Metrics Destructed"), ProfileId, TArray<FAnalyticsEventAttribute>());
}

bool FAnalyticsProviderMetrics::Tick(float DeltaSeconds)
{
	HttpRetryManager->Update();

	DeltaSecondsSinceFlush += DeltaSeconds;
	if (DeltaSecondsSinceFlush >= MaxAgeThresholdSeconds)
	{
		FlushEvents();
		DeltaSecondsSinceFlush = 0;
	}

	return true;
}

#if ENGINE_MINOR_VERSION >= 26
TSharedRef<IHttpRequest, ESPMode::ThreadSafe> FAnalyticsProviderMetrics::CreateRequest()
{
	FHttpRetrySystem::FRetryVerbs Verbs;
	FHttpRetrySystem::FRetryResponseCodes Codes;

	// Retry our POST Requests on various 5xx errors
	Verbs.Add("POST");
	Codes.Add(502);	   // Bad Gateway
	Codes.Add(503);	   // Service Unavailable
	Codes.Add(504);	   // Gateway Timeout
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = HttpRetryManager->CreateRequest(RetryLimitCount, RetryLimitTime, Codes, Verbs);

	return HttpRequest;
}
#else
TSharedRef<IHttpRequest> FAnalyticsProviderMetrics::CreateRequest()
{
	FHttpRetrySystem::FRetryVerbs Verbs;
	FHttpRetrySystem::FRetryResponseCodes Codes;

	// Retry our POST Requests on various 5xx errors
	Verbs.Add("POST");
	Codes.Add(502);	   // Bad Gateway
	Codes.Add(503);	   // Service Unavailable
	Codes.Add(504);	   // Gateway Timeout
	TSharedRef<IHttpRequest> HttpRequest = HttpRetryManager->CreateRequest(RetryLimitCount, RetryLimitTime, Codes, Verbs);

	return HttpRequest;
}
#endif

bool FAnalyticsProviderMetrics::StartSession(const TArray<FAnalyticsEventAttribute>& Attributes)
{
	// Skip starting session in automation testing.
	if (!bHasSessionStarted && !GIsAutomationTesting)
	{
		// Set the sessionID here so that SessionStart comes with an ID.
		// SetSessionID can still be used to set a new one.
		SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
		TelemetryClassEvent(TEXT("session"), TEXT("SessionStart"), ProfileId, Attributes);
		UE_LOG(LogAnalytics, Display, TEXT("Metrics::StartSession(%d attributes)"), Attributes.Num());
		bHasSessionStarted = true;
	}
	return bHasSessionStarted;
}

void FAnalyticsProviderMetrics::HandleResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) const
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogAnalytics, Warning, TEXT("Request failed to access %s"), *Request->GetURL());
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogAnalytics, Warning, TEXT("Couldn't store the data: %s"), *Response->GetContentAsString());
	}
}

void FAnalyticsProviderMetrics::EndSession()
{
	if (bHasSessionStarted)
	{
		TelemetryClassEvent(TEXT("session"), TEXT("SessionEnd"), ProfileId, TArray<FAnalyticsEventAttribute>());

		FlushEvents();
		bHasSessionStarted = false;
		UE_LOG(LogAnalytics, Display, TEXT("Metrics::EndSession"));
	}
}

void FAnalyticsProviderMetrics::FlushEvents()
{
	if (!Http)
	{
		UE_LOG(LogAnalytics, Warning, TEXT("No http found, trying to creating one"));
		Http = &FHttpModule::Get();
	}
	if (EventPayloads.Num() == 0)
	{
		return;
	}
	if (GIsAutomationTesting)
	{
		UE_LOG(LogAnalytics, Log, TEXT("Skipping Event submission while running in automation tests"));
		return;
	}

	// Don't send request if didn't set EndPointURL.
	if (!EndPointURL.IsEmpty())
	{
#if ENGINE_MINOR_VERSION >= 26
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = CreateRequest();
#else
		TSharedRef<IHttpRequest> Request = CreateRequest();
#endif

		const FString ContentString = GetStringifiedPayloads();
		Request->SetURL(EndPointURL + "&session_id=" + SessionId + "&key=" + ApiKey);
		Request->SetVerb("POST");
		Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
		Request->SetHeader("Content-Type", TEXT("application/json"));
		Request->SetContentAsString(ContentString);
		Request->ProcessRequest();
	}
	
	EventPayloads.Empty();
}

FString FAnalyticsProviderMetrics::GetStringifiedPayloads() const
{
	FString Payload = TEXT("[");
	const int32 NumberOfEvents = EventPayloads.Num();
	for (int32 Index = 0; Index < NumberOfEvents; ++Index)
	{
		const MetricsPayload CurrentEvent = EventPayloads[Index];
		Payload += CurrentEvent.Stringify();
		Payload += Index == (NumberOfEvents - 1) ? TEXT("]") : TEXT(",");
	}
	return Payload;
}

void FAnalyticsProviderMetrics::SetUserID(const FString& InUserID)
{
	TArray<FAnalyticsEventAttribute> Attributes = TArray<FAnalyticsEventAttribute>();
	const FAnalyticsEventAttribute OldIdAttribute = FAnalyticsEventAttribute(TEXT("OldUserId"), ProfileId);
	ProfileId = InUserID;

	const FAnalyticsEventAttribute NewIdAttribute = FAnalyticsEventAttribute(TEXT("NewUserId"), InUserID);
	Attributes.Add(OldIdAttribute);
	Attributes.Add(NewIdAttribute);

	TelemetryClassEvent(TEXT("session"), TEXT("SetUserId"), ProfileId, Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::SetUserID(%s)"), *ProfileId);
}

FString FAnalyticsProviderMetrics::GetUserID() const
{
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::GetUserID - returning cached id '%s'"), *ProfileId);
	return ProfileId;
}

void FAnalyticsProviderMetrics::SetGender(const FString& InGender)
{
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::SetGender(%s) - currently not supported"), *InGender);
}

void FAnalyticsProviderMetrics::SetAge(const int32 InAge)
{
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::SetAge(%d) - currently not supported"), InAge);
}

void FAnalyticsProviderMetrics::SetLocation(const FString& InLocation)
{
	FString Lat, Long;
	InLocation.Split(TEXT(","), &Lat, &Long);

	const double Latitude = FCString::Atod(*Lat);
	const double Longitude = FCString::Atod(*Long);

	UE_LOG(LogAnalytics, Display, TEXT("Parsed \"lat, long\" string in Metrics::SetLocation(%s) as \"%f, %f\" - currently not supported"), *InLocation, Latitude, Longitude);
}

FString FAnalyticsProviderMetrics::GetSessionID() const
{
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::GetSessionID - returning the id as '%s'"), *SessionId);

	return SessionId;
}

bool FAnalyticsProviderMetrics::SetSessionID(const FString& InSessionID)
{
	TArray<FAnalyticsEventAttribute> Attributes = TArray<FAnalyticsEventAttribute>();
	const FAnalyticsEventAttribute OldIdAttribute = FAnalyticsEventAttribute(TEXT("OldSessionId"), SessionId);
	SessionId = InSessionID;

	const FAnalyticsEventAttribute NewIdAttribute = FAnalyticsEventAttribute(TEXT("NewSessionId"), InSessionID);
	Attributes.Add(OldIdAttribute);
	Attributes.Add(NewIdAttribute);

	TelemetryClassEvent(TEXT("session"), TEXT("SetSessionId"), ProfileId, Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::SetSessionID - returning the id as '%s'"), *SessionId);
	return true;
}

void FAnalyticsProviderMetrics::RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (EventName.Len() == 0)
	{
		UE_LOG(LogAnalytics, Warning, TEXT("Metrics::RecordEvent - no event name, nothing to record"));
		return;
	}

	if (EventName.StartsWith(GEditorTelemetryPrefix))
	{
		// custom handling for EditorTelemetry events because those are passed from the epic side
		// and have no notion of the MetricsBlueprintLibrary event classes

		// our camelcasing is naive, so it'll camelcase abbreviation only fields (DDC -> dDC)
		// this is cribbed from MetricsBlueprintLibrary::EnumToString
		FString EventClass = StaticEnum<EMetricsClass>()->GetValueAsString(EMetricsClass::MetricsClass_EditorTelemetry);
		EventClass.RemoveFromStart("EMetricsClass::MetricsClass_");
		ConvertToCamelCase(EventClass);

		// the current format from epic -> io for editor telemetry is MetricsEditorTelemetry_EventName
		FString ParsedEventName = FString::Printf(TEXT("%s"), *EventName);
		ParsedEventName.RemoveFromStart(GEditorTelemetryPrefix);
		ConvertToCamelCase(ParsedEventName);

		// use the windows login name; this should be internal only reporting
		const FString ProfileID = UKismetSystemLibrary::GetPlatformUserName();

		TelemetryClassEvent(EventClass, ParsedEventName, ProfileID, Attributes);
	}
	else
	{
		// Pass through to our main telemetry reporting endpoint
		TelemetryClassEvent(TEXT(""), EventName, "", Attributes);
	}
}

// Adds accountid/profileid/client and server session id details as appropriate.
void FAnalyticsProviderMetrics::AugmentPayloadWithSessionDetails(MetricsPayload& PayloadData, const FString& EventProfileID)
{
	auto AugmentClientDetailsFromProfile = [&PayloadData, this](const FString& ProfileID) {
		// If we have profile info, we will add it.  We cache this information when we select a profile.
		if (const FProfileDetails* ProfileDetails = CachedProfileDetails.Find(ProfileID))
		{
			PayloadData.ProfileID = ProfileID;
			PayloadData.AccountID = ProfileDetails->AccountID;
			PayloadData.ClientSessionID = ProfileDetails->ClientSessionID;
		}
	};

	if (GWorld)
	{
		switch (GWorld->GetNetMode())
		{
			// Our two server cases.
			case NM_DedicatedServer:	// fallthrough
			case NM_ListenServer:
				// Store the profile of the character who generated this event.  Possibly blank
				PayloadData.ServerSessionID = SessionId;

				AugmentClientDetailsFromProfile(EventProfileID);
				break;

			// While we are in the main menu, before connecting as a client.  We have an account ID, but no profile id for part of this flow.
			// So if we can't find the cached profile details we skip the profile ID data and just use the account ID
			case NM_Standalone:
				if (const FProfileDetails* ProfileDetails = CachedProfileDetails.Find(EventProfileID))
				{
					PayloadData.ProfileID = EventProfileID;
					PayloadData.AccountID = ProfileDetails->AccountID;
					PayloadData.ClientSessionID = ProfileDetails->ClientSessionID;
				}
				else
				{
					PayloadData.ClientSessionID = SessionId;
					PayloadData.AccountID = EventProfileID;
				}
				break;

			// Clients connected to servers.
			case NM_Client:
				PayloadData.ClientSessionID = SessionId;
				AugmentClientDetailsFromProfile(EventProfileID);
				break;

			default:
				UE_LOG(LogAnalytics, Warning, TEXT("Unexpected netmode: %d"), GWorld->GetNetMode());
		}
	}
	else
	{
		// Happens on shutdown when we are ending our session.  We don't have a World, so add what ever details we can.
		if (IsRunningDedicatedServer())
		{
			PayloadData.ServerSessionID = SessionId;
		}
		else
		{
			AugmentClientDetailsFromProfile(EventProfileID);
		}
	}
}

void FAnalyticsProviderMetrics::TelemetryClassEvent(const FString& EventClass, const FString& EventName, const FString& EventProfileID, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	EventId++;
	MetricsPayload PayloadData;

	PayloadData.WorkerID = WorkerId;

	PayloadData.EventType = EventName;
	PayloadData.EventIndex = EventId;
	PayloadData.EventAttributes = Attributes;
	PayloadData.EngineVersion = EngineVersion;
	PayloadData.EventEnvironment = EventEnvironment;
	PayloadData.EventClass = EventClass;

	AugmentPayloadWithSessionDetails(PayloadData, EventProfileID);

	// EventSource changes as you navigate from the main screen through to connecting to a server, so
	PayloadData.EventSource = GetEventSource();
	PayloadData.EventTimestamp = FDateTime::UtcNow().ToUnixTimestamp();

	EventPayloads.Add(PayloadData);
	if (EventPayloads.Num() >= BatchSizeThreshold)
	{
		FlushEvents();
	}
	if (TelemetryReported.IsValid())
	{
		TelemetryReported->Increment(1);
	}
}

void FAnalyticsProviderMetrics::RecordItemPurchase(const FString& ItemId, const FString& Currency, int PerItemCost, int ItemQuantity)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute ItemIdAttr = FAnalyticsEventAttribute(TEXT("ItemId"), ItemId);
	const FAnalyticsEventAttribute CurrencyAttr = FAnalyticsEventAttribute(TEXT("Currency"), Currency);
	const FAnalyticsEventAttribute PerItemCostAttr = FAnalyticsEventAttribute(TEXT("PerItemCost"), FString::FromInt(PerItemCost));
	const FAnalyticsEventAttribute ItemQuantityAttr = FAnalyticsEventAttribute(TEXT("ItemQuantity"), FString::FromInt(ItemQuantity));

	Attributes.Add(ItemIdAttr);
	Attributes.Add(CurrencyAttr);
	Attributes.Add(PerItemCostAttr);
	Attributes.Add(ItemQuantityAttr);

	RecordEvent(TEXT("ItemPurchased"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordItemPurchase('%s', '%s', %d, %d)"), *ItemId, *Currency, PerItemCost, ItemQuantity);
}

void FAnalyticsProviderMetrics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute GameCurrencyTypeAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyType"), GameCurrencyType);
	const FAnalyticsEventAttribute GameCurrencyAmountAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyAmount"), FString::FromInt(GameCurrencyAmount));
	const FAnalyticsEventAttribute RealCurrencyTypeAttr = FAnalyticsEventAttribute(TEXT("RealCurrencyType"), RealCurrencyType);
	const FAnalyticsEventAttribute RealMoneyCostAttr = FAnalyticsEventAttribute(TEXT("RealMoneyCost"), FString::SanitizeFloat(RealMoneyCost));
	const FAnalyticsEventAttribute PaymentProviderAttr = FAnalyticsEventAttribute(TEXT("PaymentProvider"), PaymentProvider);

	Attributes.Add(GameCurrencyTypeAttr);
	Attributes.Add(GameCurrencyAmountAttr);
	Attributes.Add(RealCurrencyTypeAttr);
	Attributes.Add(RealMoneyCostAttr);
	Attributes.Add(PaymentProviderAttr);

	RecordEvent(TEXT("CurrencyPurchased"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordCurrencyPurchase('%s', %d, '%s', %.02f, %s)"), *GameCurrencyType, GameCurrencyAmount, *RealCurrencyType, RealMoneyCost, *PaymentProvider);
}

void FAnalyticsProviderMetrics::RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute GameCurrencyTypeAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyType"), GameCurrencyType);
	const FAnalyticsEventAttribute GameCurrencyAmountAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyAmount"), FString::FromInt(GameCurrencyAmount));

	Attributes.Add(GameCurrencyTypeAttr);
	Attributes.Add(GameCurrencyAmountAttr);

	RecordEvent(TEXT("CurrencyGiven"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordCurrencyGiven('%s', %d)"), *GameCurrencyType, GameCurrencyAmount);
}

void FAnalyticsProviderMetrics::RecordItemPurchase(const FString& ItemId, int ItemQuantity, const TArray<FAnalyticsEventAttribute>& EventAttrs)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute ItemIdAttr = FAnalyticsEventAttribute(TEXT("ItemId"), ItemId);
	const FAnalyticsEventAttribute ItemAmountAttr = FAnalyticsEventAttribute(TEXT("ItemQuantity"), FString::FromInt(ItemQuantity));

	Attributes.Append(EventAttrs);
	Attributes.Add(ItemIdAttr);
	Attributes.Add(ItemAmountAttr);

	RecordEvent(TEXT("ItemPurchased"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordItemPurchase('%s', %d, %d)"), *ItemId, ItemQuantity, Attributes.Num());
}

void FAnalyticsProviderMetrics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute GameCurrencyTypeAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyType"), GameCurrencyType);
	const FAnalyticsEventAttribute GameCurrencyAmountAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyAmount"), FString::FromInt(GameCurrencyAmount));

	Attributes.Append(EventAttrs);
	Attributes.Add(GameCurrencyTypeAttr);
	Attributes.Add(GameCurrencyAmountAttr);

	RecordEvent(TEXT("CurrencyPurchased"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordCurrencyPurchase('%s', %d, %d)"), *GameCurrencyType, GameCurrencyAmount, Attributes.Num());
}

void FAnalyticsProviderMetrics::RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute GameCurrencyTypeAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyType"), GameCurrencyType);
	const FAnalyticsEventAttribute GameCurrencyAmountAttr = FAnalyticsEventAttribute(TEXT("GameCurrencyAmount"), FString::FromInt(GameCurrencyAmount));

	Attributes.Append(EventAttrs);
	Attributes.Add(GameCurrencyTypeAttr);
	Attributes.Add(GameCurrencyAmountAttr);

	RecordEvent(TEXT("CurrencyGiven"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordCurrencyGiven('%s', %d, %d)"), *GameCurrencyType, GameCurrencyAmount, Attributes.Num());
}

void FAnalyticsProviderMetrics::RecordError(const FString& Error, const TArray<FAnalyticsEventAttribute>& EventAttrs)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute GameErrorAttr = FAnalyticsEventAttribute(TEXT("Error"), Error);

	Attributes.Append(EventAttrs);
	Attributes.Add(GameErrorAttr);

	RecordEvent(TEXT("Error"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordError('%s', %d)"), *Error, EventAttrs.Num());
}

void FAnalyticsProviderMetrics::RecordProgress(const FString& ProgressType, const FString& ProgressHierarchy, const TArray<FAnalyticsEventAttribute>& EventAttrs)
{
	TArray<FAnalyticsEventAttribute> Attributes;
	const FAnalyticsEventAttribute ProgressTypeAttr = FAnalyticsEventAttribute(TEXT("ProgressType"), ProgressType);
	const FAnalyticsEventAttribute ProgressHierarchyAttr = FAnalyticsEventAttribute(TEXT("ProgressHierarchy"), ProgressHierarchy);

	Attributes.Append(EventAttrs);
	Attributes.Add(ProgressTypeAttr);
	Attributes.Add(ProgressHierarchyAttr);

	RecordEvent(TEXT("Progress"), Attributes);
	UE_LOG(LogAnalytics, Display, TEXT("Metrics::RecordProgress('%s', %s, %d)"), *ProgressType, *ProgressHierarchy, Attributes.Num());
}

FString FAnalyticsProviderMetrics::MetricsPayload::Stringify() const
{
	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	TSharedRef<FJsonObject> Attributes = MakeShared<FJsonObject>();

	Obj->SetStringField(TEXT("eventEnvironment"), EventEnvironment);
	Obj->SetStringField(TEXT("eventSource"), EventSource);
	Obj->SetStringField(TEXT("eventClass"), EventClass);
	Obj->SetStringField(TEXT("eventType"), EventType);
	Obj->SetNumberField(TEXT("eventTimestamp"), EventTimestamp);
	Obj->SetNumberField(TEXT("eventIndex"), EventIndex);

	Obj->SetStringField(TEXT("versionId"), EngineVersion);

	if (!AccountID.IsEmpty())
	{
		Obj->SetStringField(TEXT("accountId"), AccountID);
	}
	if (!ProfileID.IsEmpty())
	{
		Obj->SetStringField(TEXT("profileId"), ProfileID);
	}
	if (!ServerSessionID.IsEmpty())
	{
		Obj->SetStringField(TEXT("serverSessionId"), ServerSessionID);
	}
	if (!ClientSessionID.IsEmpty())
	{
		Obj->SetStringField(TEXT("clientSessionId"), ClientSessionID);
	}

	if (!WorkerID.IsEmpty())
	{
		Obj->SetStringField(TEXT("workerId"), WorkerID);
	}

	FString Output;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);
	FJsonSerializer::Serialize(Obj, JsonWriter);

	return Output;
}

void FAnalyticsProviderMetrics::CacheProfileDetails(const FString& ProfileID, const FString& AccountID, const FString& ClientSessionID)
{
	CachedProfileDetails.Emplace(ProfileID, FProfileDetails{ AccountID, ClientSessionID });
}
