// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestRunner.h"

#include "UnrealNetwork.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"

#include "GDKTestGyms/ReplicationTests/TestSuiteCharacter.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestIntReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestFloatReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestBoolReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestFStringReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestCArrayReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestTArrayReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestEnumReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestFTextReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestFNameReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestUStructReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestUObjectReplication.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestMulticastRPC.h"
#include "GDKTestGyms/ReplicationTests/Tests/TestStaticComponentReplication.h"

DEFINE_LOG_CATEGORY(LogSpatialGDKTestSuite);

AGDKTestRunner::AGDKTestRunner()
	: bIsRunning(false)
	, CurrentTestIndex(0)
	, ReadyClientsCount(0)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
}

// Called every frame
void AGDKTestRunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsRunning)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (GetNetMode() != NM_DedicatedServer || !GameMode || !(ReadyClientsCount == GameMode->GetNumPlayers()))
		{
			return;
		}

		// execute the tests
		if (TestCases[CurrentTestIndex]->IsFinished())
		{
			CurrentTestIndex++;

			if (CurrentTestIndex == TestCases.Num())
			{
				bIsRunning = false;
				ReadyClientsCount = 0;
				CurrentTestIndex = 0;
				Server_TearDownTestCases();
				UE_LOG(LogSpatialGDKTestSuite, Log, TEXT("TestRunner: All Tests completed successfully!"));
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("TestRunner: All Tests completed successfully!"), true, FVector2D{1.5f, 1.5f});
			}
			else
			{
				TestCases[CurrentTestIndex]->Server_StartTest();
			}
		}
	}
}

bool AGDKTestRunner::Server_RunTests_Validate()
{
	return true;
}

void AGDKTestRunner::Server_RunTests_Implementation()
{
	if (!bIsRunning)
	{
		ReadyClientsCount = 0;
		bIsRunning = true;

		Server_SetupTestCases();
	}
	else
	{
		UE_LOG(LogSpatialGDKTestSuite, Log, TEXT("TestRunner: Could not start the test runner, it is already running."));
	}

}

bool AGDKTestRunner::IsRunning() const
{
	return bIsRunning;
}

void AGDKTestRunner::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGDKTestRunner, TestCases, COND_None);
	DOREPLIFETIME_CONDITION(AGDKTestRunner, bIsRunning, COND_None);
}

bool AGDKTestRunner::Server_SignalClientReady_Validate(AGDKTestRunner* SendingTestRunner)
{
	return true;
}

void AGDKTestRunner::Server_SignalClientReady_Implementation(AGDKTestRunner* SendingTestRunner)
{
	if(SendingTestRunner == this)
	{
		ReadyClientsCount++;
		UE_LOG(LogTemp, Log, TEXT("Server_SignalClientReady_Implementation: ReadyClientsCount: %d"), ReadyClientsCount);

		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (!GameMode || !(ReadyClientsCount == GameMode->GetNumPlayers()))
		{
			return;
		}

		// Start the new test case as we are ready
		CurrentTestIndex = 0;
		TestCases[CurrentTestIndex]->Server_StartTest();
	}
	else
	{
		SendingTestRunner->Server_SignalClientReady_Implementation(SendingTestRunner);
	}
}

void AGDKTestRunner::Server_SetupTestCases()
{
	// Setup the test cases here.
	if (GetNetMode() != NM_DedicatedServer)
	{
		return;
	}

	AddTestCase<ATestIntReplication>();
	AddTestCase<ATestFloatReplication>();
	AddTestCase<ATestBoolReplication>();
	AddTestCase<ATestFStringReplication>();
	AddTestCase<ATestCArrayReplication>();
	AddTestCase<ATestTArrayReplication>();
	AddTestCase<ATestEnumReplication>();
	AddTestCase<ATestFTextReplication>();
	AddTestCase<ATestFNameReplication>();
	AddTestCase<ATestUObjectReplication>();
	AddTestCase<ATestUStructReplication>();
	AddTestCase<ATestMulticastRPC>();
	AddTestCase<ATestStaticComponentReplication>();
}

void AGDKTestRunner::Server_TearDownTestCases()
{
	for (AGDKTestCase* TestCase : TestCases)
	{
		TestCase->Server_TearDown();
	}

	TestCases.Empty();
}

void AGDKTestRunner::OnRep_TestCases()
{
	bool bReadyForTests = true;
	if (GetNetMode() == NM_Client)
	{
		for (auto TestCase : TestCases)
		{
			if (TestCase == nullptr)
			{
				bReadyForTests = false;
			}
		}
	}

	if (TestCases.Num() > 0 && bReadyForTests)
	{
		GetOwnedTestRunnerOnThisClient()->Server_SignalClientReady(this);
	}
}

template<typename T>
void AGDKTestRunner::AddTestCase()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	T* TestCase = World->SpawnActor<T>();
	AGDKTestCase* GDKTestCase = Cast<AGDKTestCase>(TestCase);
	check(GDKTestCase);

	TArray<AGDKTestRunner*> Runners;
	for (auto It = World->GetPlayerControllerIterator(); It; It++)
	{
		ATestSuiteCharacter* Character = Cast<ATestSuiteCharacter>((*It)->GetPawn());
		check(Character);
		Runners.Add(Character->TestRunner);
	}
	GDKTestCase->InitializeGDKTestRunnersForThisTestCase(Runners);
	TestCases.Add(GDKTestCase);
}

AGDKTestRunner * AGDKTestRunner::GetOwnedTestRunnerOnThisClient()
{
	ATestSuiteCharacter* Character = Cast<ATestSuiteCharacter>(GetWorld()->GetFirstLocalPlayerFromController()->GetPlayerController(GetWorld())->GetPawn());
	check(Character);
	return Character->TestRunner;
}
