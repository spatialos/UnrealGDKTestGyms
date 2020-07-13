#include "NFRConstants.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineUtils.h"
#include "GDKTestGymsGameInstance.h"
#include "Interop/SpatialWorkerFlags.h"

DEFINE_LOG_CATEGORY(LogNFRConstants);

FMetricTimer::FMetricTimer(int32 Seconds)
{
	SetTimer(Seconds);
}

void FMetricTimer::SetTimer(int32 Seconds)
{
	TimeToStart = FDateTime::Now().GetTicks() + FTimespan::FromSeconds(Seconds).GetTicks();
}

bool FMetricTimer::HasTimerGoneOff() const
{
	return FDateTime::Now().GetTicks() > TimeToStart;
}

int32 FMetricTimer::GetSecondsRemaining() const
{
	FTimespan Timespan = FTimespan(TimeToStart - FDateTime::Now().GetTicks());
	int32 SecondsRemaining = Timespan.GetTotalSeconds();
	SecondsRemaining = SecondsRemaining < 0 ? 0 : SecondsRemaining;
	return SecondsRemaining;
}


UNFRConstants::UNFRConstants()
	: PlayerCheckMetricDelay(10 * 60)
	, ServerFPSMetricDelay(60)
	, ClientFPSMetricDelay(60)
	, UXMetricDelay(10 * 60)
	, ActorCheckDelay(10 * 60)
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

const UNFRConstants* UNFRConstants::Get(const UWorld* World)
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
