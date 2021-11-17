// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "Interop/Connection/SpatialEventTracer.h"
#include "Interop/Connection/SpatialGDKSpanId.h"

#include "LatencyTracer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLatencyTracer, Log, All);

UCLASS(BlueprintType)
class ULatencyTracer : public UObject
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable)
	void InitTracer();

	UFUNCTION(BlueprintCallable)
	FUserSpanId BeginLatencyTrace(const FString& Type);

	UFUNCTION(BlueprintCallable)
	FUserSpanId ContinueLatencyTrace(const FString& Type, const FUserSpanId& Span);

	UFUNCTION(BlueprintCallable)
	void EndLatencyTrace(const FString& Type, const FUserSpanId& Span);

private:
	FSpatialGDKSpanId EmitTrace(const FString& EventType, FSpatialGDKSpanId* Causes, uint32 NumCauses);

	TUniquePtr<SpatialGDK::SpatialEventTracer> LocalTracer;
	SpatialGDK::SpatialEventTracer* InternalTracer = nullptr;
	FString WorkerId;
};
