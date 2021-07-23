// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "LatencyPayload.h"
#include "Interop/Connection/SpatialEventTracer.h"
#include "Interop/Connection/SpatialGDKSpanId.h"
#include "Utils/GDKPropertyMacros.h"

#include "LatencyTracer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLatencyTracer, Log, All);

UCLASS(BlueprintType)
class ULatencyTracer : public UObject
{
	GENERATED_BODY()
public:
	ULatencyTracer();

	UFUNCTION(BlueprintCallable, Category = "SpatialOS", meta = (WorldContext = "WorldContextObject"))
	static ULatencyTracer* GetOrCreateLatencyTracer(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	void InitTracer();

	UFUNCTION(BlueprintCallable)
	FUserSpanId BeginLatencyTrace(const FString& Type);

	UFUNCTION(BlueprintCallable)
	FUserSpanId ContinueLatencyTrace(const FString& Type, const FUserSpanId& Span);

	UFUNCTION(BlueprintCallable)
	void EndLatencyTrace(const FString& Type, const FUserSpanId& Span);

	FSpatialGDKSpanId EmitTrace(const FString& EventType, FSpatialGDKSpanId* Causes, uint32 NumCauses);

	TUniquePtr<SpatialGDK::SpatialEventTracer> LocalTracer;
	SpatialGDK::SpatialEventTracer* InternalTracer = nullptr;
	FString WorkerId;
};
