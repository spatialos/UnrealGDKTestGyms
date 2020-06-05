#include "NFRConstants.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "Interop/SpatialWorkerFlags.h"

DEFINE_LOG_CATEGORY(LogNFRConstants);

bool NFRConstants::SamplesForFPSValid()
{
	if (bFPSSamplingValid)
	{
		return true;
	}
	bool bIsValidNow = FDateTime::Now().GetTicks() > TimeToStartFPSSampling;
	if (bIsValidNow)
	{
		bFPSSamplingValid = true;
	}
	return bIsValidNow;
}

void NFRConstants::Init(UWorld* World)
{
	TimeToStartFPSSampling = FDateTime::Now().GetTicks() + FTimespan::FromSeconds(120.0f).GetTicks();

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver());
	// Sever fps
	{
		float OverrideMinServerFPS;
		if (FParse::Value(FCommandLine::Get(), TEXT("MinServerFPS="), OverrideMinServerFPS))
		{
			MinServerFPS = OverrideMinServerFPS;
		}
	}
	{
		FString MinServerFPSStr;
		if (NetDriver != nullptr && NetDriver->SpatialWorkerFlags != nullptr && NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("min_server_fps"), MinServerFPSStr))
		{
			MinServerFPS = FCString::Atof(*MinServerFPSStr);
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
	}
	{
		FString MinClientFPSStr;
		if (NetDriver != nullptr && NetDriver->SpatialWorkerFlags != nullptr && NetDriver->SpatialWorkerFlags->GetWorkerFlag(TEXT("min_client_fps"), MinClientFPSStr))
		{
			MinClientFPS = FCString::Atof(*MinClientFPSStr);
		}
	}
	UE_LOG(LogNFRConstants, Log, TEXT("Min client FPS: %.8f."), MinClientFPS);
	bInitialized = true;
}

NFRConstants& NFRConstants::Get()
{
	static NFRConstants _;
	return _;
}

float NFRConstants::GetMinServerFPS() const
{
	check(bInitialized); 
	return MinServerFPS;
}

float NFRConstants::GetMinClientFPS() const
{
	check(bInitialized); 
	return MinClientFPS;
}
