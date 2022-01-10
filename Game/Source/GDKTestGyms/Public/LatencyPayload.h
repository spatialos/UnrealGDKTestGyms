// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Containers/Array.h"

#include "LatencyPayload.generated.h"

USTRUCT(BlueprintType)
struct FLatencyPayload
{
	GENERATED_BODY()
	
	FLatencyPayload() {} // To keep reflection happy

	FLatencyPayload(TArray<uint8>&& Data)
		: Data(MoveTemp(Data))
	{
	}

	UPROPERTY()
	TArray<uint8> Data;

// 	void SetSpan(const FSpatialGDKSpanId& SpanId)
// 	{
// 		Data.SetNum(sizeof(FSpatialGDKSpanId));
// 		memcpy(&Data[0], SpanId.GetConstId(), sizeof(FSpatialGDKSpanId));
// 	}
};
