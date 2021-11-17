// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LatencyTracer.h"

#include "Engine/World.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "SpatialGDKSettings.h"
#include "Utils/SpatialStatics.h"

#include "GDKTestGyms/GDKTestGymsGameInstance.h"

DEFINE_LOG_CATEGORY(LogLatencyTracer);

namespace LatencyTracerInternal
{
	SpatialGDK::SpatialEventTracer* GetEventTracerFromNetDriver(const UWorld* World)
	{
		if (USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver()))
		{
			return NetDriver->Connection->GetEventTracer();
		}

		return nullptr;
	}
}

ULatencyTracer* ULatencyTracer::GetOrCreateLatencyTracer(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		World = GWorld;
	}

	if (UGDKTestGymsGameInstance* GameInstance = World->GetGameInstance<UGDKTestGymsGameInstance>())
	{
		if (GameInstance->GetTracer() != nullptr)
		{
			return GameInstance->GetTracer();
		}
	}
	UE_LOG(LogLatencyTracer, Log, TEXT("Unable to get the game instance tracer object, creating a new one but results may not be as expected."));
	return NewObject<ULatencyTracer>();
}

void ULatencyTracer::InitTracer()
{
	// TODO: Also remove references to TraceMetadata + LatencyGameMode metadata id (run id?)
	const USpatialGDKSettings* Settings = GetDefault<USpatialGDKSettings>();
	if (Settings->GetEventTracingEnabled() && USpatialStatics::IsSpatialNetworkingEnabled()) // Use the tracing hooked up to GDK internals
	{
		InternalTracer = LatencyTracerInternal::GetEventTracerFromNetDriver(GetWorld());
		if (InternalTracer == nullptr)
		{
			UE_LOG(LogLatencyTracer, Warning, TEXT("Full tracing enabled but tracer is not available."));
		}
	}
	else
	{
		ENetMode NetMode = GetWorld()->GetNetMode();
		bool bIsClient = NetMode == NM_Client || NetMode == NM_Standalone;

		WorkerId = (bIsClient ? TEXT("UnrealClient") : TEXT("UnrealWorker")) + FGuid::NewGuid().ToString();
		
		UEventTracingSamplingSettings* SamplingSettings = NewObject<UEventTracingSamplingSettings>();
		SamplingSettings->SamplingProbability = 1.0f;
		SamplingSettings->GDKEventPreFilter = TEXT("true");
		SamplingSettings->GDKEventPostFilter = TEXT("true");
		LocalTracer = MakeUnique<SpatialGDK::SpatialEventTracer>(WorkerId, SamplingSettings);
		InternalTracer = LocalTracer.Get();
	}
}

FUserSpanId ULatencyTracer::BeginLatencyTrace(const FString& Type)
{
	FSpatialGDKSpanId Span = EmitTrace(Type, nullptr, 0);
	return FSpatialGDKSpanId::ToUserSpan(Span);
}

FUserSpanId ULatencyTracer::ContinueLatencyTrace(const FString& Type, const FUserSpanId& SpanIn)
{
	FSpatialGDKSpanId Cause = FSpatialGDKSpanId::FromUserSpan(SpanIn);
	FSpatialGDKSpanId SpanOut = EmitTrace(Type, &Cause, 1);
	return FSpatialGDKSpanId::ToUserSpan(SpanOut);
}

void ULatencyTracer::EndLatencyTrace(const FString& Type, const FUserSpanId& SpanIn)
{
	FSpatialGDKSpanId Cause= FSpatialGDKSpanId::FromUserSpan(SpanIn);
	EmitTrace(Type, (FSpatialGDKSpanId*)&Cause, 1);
}

FSpatialGDKSpanId ULatencyTracer::EmitTrace(const FString& EventType, FSpatialGDKSpanId* Causes, uint32 NumCauses)
{
	if (InternalTracer != nullptr)
	{
		return InternalTracer->TraceEvent(TCHAR_TO_ANSI(*EventType), "", (Trace_SpanIdType*)Causes, NumCauses,
								   [this](SpatialGDK::FSpatialTraceEventDataBuilder& Builder) {
									   Builder.AddKeyValue("worker_id", WorkerId);
								   });
	}
	return {};
}
