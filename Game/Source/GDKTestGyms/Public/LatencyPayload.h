// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Hash/CityHash.h"
#include "Interop/Connection/SpatialGDKSpanId.h"
#include "SpatialCommonTypes.h"

#include "LatencyPayload.generated.h"

USTRUCT(BlueprintType)
struct FLatencyPayload
{
	GENERATED_BODY()

	FLatencyPayload() {}

	FLatencyPayload(TArray<uint8>&& Data)
		: Data(MoveTemp(Data))
	{
	}

	UPROPERTY()
	TArray<uint8> Data;

	// Required for TMap hash
	bool operator==(const FLatencyPayload& Other) const { return Data == Other.Data; }

	friend uint32 GetTypeHash(const FLatencyPayload& Obj) // TODO: Why is this needed?
	{
		return CityHash32((const char*)Obj.Data.GetData(), Obj.Data.Num());
	}

	void SetSpan(const FSpatialGDKSpanId& SpanId)
	{
		Data.SetNum(sizeof(FSpatialGDKSpanId));
		memcpy(&Data[0], SpanId.GetConstId(), sizeof(FSpatialGDKSpanId));
	}
};
