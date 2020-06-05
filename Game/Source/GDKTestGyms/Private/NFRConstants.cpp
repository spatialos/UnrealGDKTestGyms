#include "NFRConstants.h"

NFRConstants::NFRConstants()
{
	// Cmd line overrides
	TimeToStartFPSSampling = FDateTime::Now().GetTicks() + FTimespan::FromSeconds(120.0f).GetTicks();
}

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

NFRConstants& NFRConstants::Get()
{
	static NFRConstants _;
	return _;
}
