// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LatencyTracer.h"

#include "Async/Async.h"
#include "Engine/World.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "GeneralProjectSettings.h"
#include "Interop/Connection/OutgoingMessages.h"
#include "SpatialGDKSettings.h"
#include "Utils/GDKPropertyMacros.h"
#include "Utils/SchemaUtils.h"

ULatencyTracer::ULatencyTracer()
{
	ResetWorkerId();
	// TODO: Also remove references to TraceMetadata + LatencyGameMode metadata id (run id?)
}

void ULatencyTracer::Setup(const FString& FallbackWorkerName)
{
	const USpatialGDKSettings* Settings = GetDefault<USpatialGDKSettings>();
	if (Settings->bEventTracingEnabled) // Use the tracing hooked up to GDK internals
	{
		// TODO
	}
	else
	{
		InternalTracer = MakeShared<SpatialGDK::SpatialEventTracer>(FallbackWorkerName);
	}
}

void ULatencyTracer::RegisterProject(const FString& ProjectId)
{
	// TODO: Remove
}

bool ULatencyTracer::BeginLatencyTrace(const FString& TraceDesc, FLatencyPayload& OutLatencyPayload)
{
	FSpatialGDKSpanId Span = EmitTrace("latency.begin", TraceDesc, nullptr, 0);
	OutLatencyPayload.SetSpan(Span);
	return true;
}

bool ULatencyTracer::ContinueLatencyTraceRPC(const AActor* Actor, const FString& FunctionName, const FString& TraceDesc,
													const FLatencyPayload& LatencyPayload,
													FLatencyPayload& OutContinuedLatencyPayload)
{
	return ContinueLatencyTrace_Internal(Actor, FunctionName, ETraceType::RPC, TraceDesc, LatencyPayload, OutContinuedLatencyPayload);
}

bool ULatencyTracer::ContinueLatencyTraceProperty(const AActor* Actor, const FString& PropertyName, const FString& TraceDesc,
														 const FLatencyPayload& LatencyPayload,
	FLatencyPayload& OutContinuedLatencyPayload)
{
	return ContinueLatencyTrace_Internal(Actor, PropertyName, ETraceType::Property, TraceDesc, LatencyPayload, OutContinuedLatencyPayload);
}

bool ULatencyTracer::ContinueLatencyTraceTagged(const AActor* Actor, const FString& Tag, const FString& TraceDesc,
													   const FLatencyPayload& LatencyPayload,
	FLatencyPayload& OutContinuedLatencyPayload)
{
	return ContinueLatencyTrace_Internal(Actor, Tag, ETraceType::Tagged, TraceDesc, LatencyPayload, OutContinuedLatencyPayload);
}

bool ULatencyTracer::EndLatencyTrace(const FLatencyPayload& LatencyPayload)
{
	FSpatialGDKSpanId SpanId((const Trace_SpanIdType*)LatencyPayload.Data.GetData());
	FSpatialGDKSpanId Span = EmitTrace("latency.end", "", &SpanId, 1);
	return true;
}

void ULatencyTracer::ResetWorkerId()
{
	WorkerId = TEXT("DeviceId_") + FPlatformMisc::GetDeviceId();
}

bool ULatencyTracer::ContinueLatencyTrace_Internal(const AActor* Actor, const FString& Target, ETraceType::Type Type,
														  const FString& TraceDesc, const FLatencyPayload& LatencyPayload,
	FLatencyPayload& OutLatencyPayload)
{
	const char* EventType = [](auto Type) -> const char* {
		switch (Type)
		{
		case ETraceType::Type::RPC:
			return "latency.continue_rpc";
		case ETraceType::Property:
			return "latency.continue_property";
		case ETraceType::Tagged:
			return "latency.continue_tagged";
		};
		return "unknown";
	}(Type);

	FSpatialGDKSpanId SpanId((const Trace_SpanIdType*)LatencyPayload.Data.GetData());
	FSpatialGDKSpanId Span = EmitTrace(EventType, TraceDesc, &SpanId, 1);
	OutLatencyPayload.SetSpan(Span);
	return true;
}

FSpatialGDKSpanId ULatencyTracer::EmitTrace(FString EventType, FString Message, FSpatialGDKSpanId* Causes, uint32 NumCauses)
{
	if (InternalTracer != nullptr)
	{
		InternalTracer->TraceEvent(TCHAR_TO_ANSI(*EventType), TCHAR_TO_ANSI(*Message), (Trace_SpanIdType*)Causes, NumCauses,
								   [this](SpatialGDK::FSpatialTraceEventDataBuilder& Builder) {
									   Builder.AddKeyValue("worker_id", WorkerId);
								   });
	}
	return {};
}
