#include "NFRConstants.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineUtils.h"
#include "GDKTestGymsGameInstance.h"
#include "Interop/SpatialWorkerFlags.h"

DEFINE_LOG_CATEGORY(LogNFRConstants);


FMetricDelay::FMetricDelay()
	: bIsReady(false)
	, TimeToStart(0)
{
}

FMetricDelay::FMetricDelay(int64 InTimeToStart)
	: bIsReady(false)
	, TimeToStart(InTimeToStart)
{
}

bool FMetricDelay::IsReady()
{
	if (bIsReady)
	{
		return true;
	}

	if (FDateTime::Now().GetTicks() > TimeToStart)
	{
		bIsReady = true;
	}

	return bIsReady;
}

UNFRConstants::UNFRConstants()
	: PlayerCheckMetricDelay(FDateTime::Now().GetTicks() + FTimespan::FromSeconds(15.0f * 60.0f).GetTicks())
	, ServerFPSMetricDelay(FDateTime::Now().GetTicks() + FTimespan::FromSeconds(2.0f * 60.0f).GetTicks())
	, ClientFPSMetricDelay(FDateTime::Now().GetTicks() + FTimespan::FromSeconds(10.0f * 60.0f).GetTicks())
	, UXMetricDelay(FDateTime::Now().GetTicks() + FTimespan::FromSeconds(10.0f * 60.0f).GetTicks())
{
}

void UNFRConstants::InitWithWorld(const UWorld* World)
{
	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver());
	// Server fps
	{
		float OverrideMinServerFPS;
		if (FParse::Value(FCommandLine::Get(), TEXT("MinServerFPS="), OverrideMinServerFPS))
		{
			MinServerFPS = OverrideMinServerFPS;
		}
		else
		{
			FString MinServerFPSStr;
			if (NetDriver != nullptr && NetDriver->SpatialWorkerFlags != nullptr && NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("min_server_fps"), MinServerFPSStr))
			{
				MinServerFPS = FCString::Atof(*MinServerFPSStr);
			}
		}
	}
	UE_LOG(LogNFRConstants, Log, TEXT("Min server FPS: %.8f."), MinServerFPS);
	// Client fps
	{
		float OverrideMinClientFPS;
		if (FParse::Value(FCommandLine::Get(), TEXT("MinClientFPS="), OverrideMinClientFPS))
		{
			MinClientFPS = OverrideMinClientFPS;
		}
		else
		{
			FString MinClientFPSStr;
			if (NetDriver != nullptr && NetDriver->SpatialWorkerFlags != nullptr && NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("min_client_fps"), MinClientFPSStr))
			{
				MinClientFPS = FCString::Atof(*MinClientFPSStr);
			}
		}
	}
	UE_LOG(LogNFRConstants, Log, TEXT("Min client FPS: %.8f."), MinClientFPS);
	bIsInitialised = true;
}

UNFRConstants* UNFRConstants::Get(const UWorld* World)
{
	if (UGDKTestGymsGameInstance* GameInstance = World->GetGameInstance<UGDKTestGymsGameInstance>())
	{
		return GameInstance->GetNFRConstants();
	}
	return nullptr;
}

float UNFRConstants::GetMinServerFPS() const
{
	checkf(bIsInitialised, TEXT("NFRConstants not initialised"));
	return MinServerFPS;
}

float UNFRConstants::GetMinClientFPS() const
{
	checkf(bIsInitialised, TEXT("NFRConstants not initialised"));
	return MinClientFPS;
}
