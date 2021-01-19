// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GC_SignalCueActivation.h"
#include "CuesGASTestPawn.h"
#include "LogGymsSpatialFunctionalTest.h"

UGC_SignalCueActivation::UGC_SignalCueActivation() {}

void UGC_SignalCueActivation::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType,
												const FGameplayCueParameters& Parameters)
{
	if (ACuesGASTestPawn* TestActor = Cast<ACuesGASTestPawn>(MyTarget))
	{
		if (EventType == EGameplayCueEvent::OnActive)
		{
			TestActor->SignalOnActive();
		}
		else if (EventType == EGameplayCueEvent::Executed)
		{
			TestActor->SignalExecute();
		}
	}
	else
	{
		UE_LOG(LogGymsSpatialFunctionalTest, Error, TEXT("GC_SignalCueActivation, target actor %s is not an ACuesGASTestPawn."),
			   *GetNameSafe(MyTarget));
	}
}
