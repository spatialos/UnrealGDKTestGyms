#include "AUptimeGymsCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "AUptimeGameMode.h"

AUptimeGymsCharacter::AUptimeGymsCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TestDataSize(0)
	, TestDataFrequency(0)
{
}

void AUptimeGymsCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	GenerateWorkerFlag();
	GenerateTestData(RPCsEgressTest, TestDataSize);
}

void AUptimeGymsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	EgressTestForRPC();
}

void AUptimeGymsCharacter::GenerateWorkerFlag()
{
	UWorld* world;
	GetCurrentWorld(world);
	AUptimeGameMode* UptimeGameMode = dynamic_cast<AUptimeGameMode*>(UGameplayStatics::GetGameMode(world));
	if (nullptr != UptimeGameMode)
	{
		TestDataSize = UptimeGameMode->GetEgressTestSize();
		TestDataFrequency = UptimeGameMode->GetEgressTestNum();
	}
}

void AUptimeGymsCharacter::GenerateTestData(TArray<int32>& TestData, int32 DataSize)
{
	while (--DataSize >= 0)
	{
		srand(DataSize);
		TestData.Add(rand());
	}
}

void AUptimeGymsCharacter::EgressTestForRPC()
{
	if (HasAuthority())
	{
		RPCsForAuthority();
	}
}

void AUptimeGymsCharacter::RPCsForAuthority()
{
	ServerToClientRPCs();
}

bool AUptimeGymsCharacter::GetCurrentWorld(UWorld*& world)
{
	world = GetWorld();
	return world != nullptr;
}

void AUptimeGymsCharacter::ServerToClientRPCs()
{
	for (auto i = 1; i <= TestDataFrequency; ++i)
	{
		for (auto j = 0; j < TestDataSize; ++j)
		{
			ReportAuthoritativeServers(RPCsEgressTest[j]);
		}
	}
}

void AUptimeGymsCharacter::ReportAuthoritativeServers_Implementation(int32 TestData)
{
	auto copyTestData = TestData;
}
