// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#pragma once

#include "GDKTestGymsCharacter.h"
#include "AUptimeGymsCharacter.generated.h"

UCLASS(config = Game)
class AUptimeGymsCharacter :
    public AGDKTestGymsCharacter
{
	GENERATED_BODY()
public:
	AUptimeGymsCharacter(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	UFUNCTION(Client,Reliable)
	virtual void ReportAuthoritativeServers(int32 TestData);

private:
	void GenerateWorkerFlag();
	void GenerateTestData(TArray<int32>& TestData, int32 DataSize);
	void EgressTestForRPC();
	void RPCsForAuthority();
	bool GetCurrentWorld(UWorld*& world);
	void ServerToClientRPCs();
	int32 TestDataSize;
	int32 TestDataFrequency;
	TArray<int32> RPCsEgressTest;
};

