// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "UptimeCrossServerBeacon.h"

DEFINE_LOG_CATEGORY(LogUptimeCrossServerBeacon);

AUptimeCrossServerBeacon::AUptimeCrossServerBeacon()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	CrossServerSize = 0;
	CrossServerFrequency = 0;
	bHasReceivedSize = false;
	bHasReceivedFrequency = false;
	bHasGenerateData = false;
}

void AUptimeCrossServerBeacon::GenerateTestData(TArray<int32>& TestData, int32 DataSize)
{
	srand(DataSize);
	while (--DataSize >= 0)
	{
		TestData.Add(rand());
	}
	FrequencyTimer.SetTimer(CrossServerFrequency);
	bHasGenerateData = true;
}

void AUptimeCrossServerBeacon::Tick(float DeltaSeconds)
{  
	Super::Tick(DeltaSeconds);

	if (bHasReceivedSize && bHasReceivedFrequency)
	{
		if (!bHasGenerateData)
		{
			GenerateTestData(CrossServerData, CrossServerSize);
		}
		else
		{
			RPCsForNonAuthority();
		}
	}
}

void AUptimeCrossServerBeacon::RPCsForNonAuthority()
{
	if (!HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World != nullptr && World->IsServer())
		{
			if (FrequencyTimer.HasTimerGoneOff())
			{
				CrossServerRPC();
				FrequencyTimer.SetTimer(CrossServerFrequency);
			}
		}
	}
}

void AUptimeCrossServerBeacon::CrossServerRPC()
{
	for (auto i = 0; i < CrossServerSize; ++i)
	{
		SendCrossServer(CrossServerData[i]);
	}
}

void AUptimeCrossServerBeacon::SendCrossServer_Implementation(int32 TestData)
{
	auto TempTestData = TestData;
	UE_LOG(LogUptimeCrossServerBeacon, Log, TEXT("Send CrossServer Data:%d"), TempTestData);
}
