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

DEFINE_LOG_CATEGORY(LogLatencyTracer);

namespace LatencyTracerInternal
{
	FUserSpanId GDKSpanToUser(const FSpatialGDKSpanId& Span) // TODO: Expose the internal GDK versions of these
	{
		FUserSpanId UserSpan;
		UserSpan.Data.SetNum(sizeof(Span));
		memcpy(UserSpan.Data.GetData(), &Span, sizeof(FSpatialGDKSpanId));
		return UserSpan;
	}
	FSpatialGDKSpanId UserSpanToGDK(const FUserSpanId& UserSpan) // TODO: Expose the internal GDK versions of these
	{
		if (ensure(sizeof(FSpatialGDKSpanId) == UserSpan.Data.Num()))
		{
			return FSpatialGDKSpanId((const Trace_SpanIdType*)UserSpan.Data.GetData());
		}
		UE_LOG(LogLatencyTracer, Warning, TEXT("Expected UserSpan size to equal sizeof(FSpatialGDKSpanId)"))
		return {};
	}
}

ULatencyTracer::ULatencyTracer()
{
}

void ULatencyTracer::InitTracer()
{
	// TODO: Also remove references to TraceMetadata + LatencyGameMode metadata id (run id?)
	const USpatialGDKSettings* Settings = GetDefault<USpatialGDKSettings>();
	if (FParse::Param(FCommandLine::Get(), TEXT("--full-tracing-enabled")) && USpatialStatics::IsSpatialNetworkingEnabled()) // Use the tracing hooked up to GDK internals
	{
		// TODO
		// TODO WorkerId
	}
	else
	{
		WorkerId = FGuid::NewGuid().ToString();
		SpatialGDK::TraceQueries Queries;
		Queries.EventPreFilter = "true";
		Queries.EventPostFilter = "true";
		InternalTracer = MakeShared<SpatialGDK::SpatialEventTracer>(WorkerId, Queries);
	}
}

FUserSpanId ULatencyTracer::BeginLatencyTrace(const FString& Type)
{
	FSpatialGDKSpanId Span = EmitTrace(Type, nullptr, 0);
	return LatencyTracerInternal::GDKSpanToUser(Span);
}

FUserSpanId ULatencyTracer::ContinueLatencyTrace(const FString& Type, const FUserSpanId& SpanIn)
{
	FSpatialGDKSpanId Cause = LatencyTracerInternal::UserSpanToGDK(SpanIn);
	FSpatialGDKSpanId SpanOut = EmitTrace(Type, (FSpatialGDKSpanId*)&Cause, 1);
	return LatencyTracerInternal::GDKSpanToUser(SpanOut);
}

void ULatencyTracer::EndLatencyTrace(const FString& Type, const FUserSpanId& SpanIn)
{
	FSpatialGDKSpanId Cause = LatencyTracerInternal::UserSpanToGDK(SpanIn);
	EmitTrace(Type, (FSpatialGDKSpanId*)&Cause, 1);
}

#if 0
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
#endif

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
