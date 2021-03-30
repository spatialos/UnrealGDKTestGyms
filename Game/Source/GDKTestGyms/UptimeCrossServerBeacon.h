// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "NFRConstants.h"
#include "GameFramework/Character.h"
#include "UptimeCrossServerBeacon.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUptimeCrossServerBeacon, Log, All);

UCLASS()
class AUptimeCrossServerBeacon :
	public ACharacter
{
	GENERATED_BODY()

public:

	AUptimeCrossServerBeacon();

	void SetCrossServerSize(int32 Size) { CrossServerSize = Size; bHasReceivedSize = true; }
	void SetCrossServerFrequency(int32 Frequency) { CrossServerFrequency = Frequency; bHasReceivedFrequency = true; }

	UFUNCTION(CrossServer, Reliable)
	void SendCrossServer(int32 TestData);

private:
	int32 CrossServerSize;
	int32 CrossServerFrequency;
	TArray<int32> CrossServerData;
	bool bHasReceivedSize;
	bool bHasReceivedFrequency;
	bool bHasGenerateData;
	FMetricTimer FrequencyTimer;

	virtual void Tick(float DeltaSeconds) override;

	void GenerateTestData(TArray<int32>& TestData, int32 DataSize);
	void RPCsForNonAuthority();
	void CrossServerRPC();
};

