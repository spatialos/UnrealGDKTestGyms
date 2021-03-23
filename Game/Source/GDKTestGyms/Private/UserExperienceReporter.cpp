// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "UserExperienceReporter.h"

#include "GDKTestGyms/GDKTestGymsGameInstance.h"
#include "NFRConstants.h"
#include "Net/UnrealNetwork.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "UserExperienceComponent.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogUserExperienceReporter);

namespace
{
	float CalculateAverage(const TArray<float>& Array)
	{
		float Avg = 0.0f;
		for (int i = 0; i < Array.Num(); i++)
		{
			Avg += Array[i];
		}
		Avg /= static_cast<float>(Array.Num()) + 0.00001f;
		return Avg;
	}
}

// Sets default values for this component's properties
UUserExperienceReporter::UUserExperienceReporter()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UUserExperienceReporter::InitializeComponent()
{
	UActorComponent::InitializeComponent();
	bFrameRateValid = true;
	ServerRTTMS = 0.0f;
	ServerUpdateTimeDeltaMS = 0.0f;
}

void UUserExperienceReporter::ReportMetrics()
{
	if (UUserExperienceComponent* UXComp = Cast<UUserExperienceComponent>(GetOwner()->FindComponentByClass(UUserExperienceComponent::StaticClass())))
	{
		float RoundTripTimeMS = 0.0f;
		float UpdateTimeDeltaMS = 0.0f;

		// Round trip
		if (UXComp->RoundTripTime.Num() == UUserExperienceComponent::NumWindowSamples) // Only start reporting once window is filled.
		{
			RoundTripTimeMS = CalculateAverage(UXComp->RoundTripTime);
		}

		const UWorld* World = GetWorld();
		check(World);

		// Update time delta
		{
			bool bValidResult = true;
			int32 UpdateTimeDeltaCount = 0;
			for (TObjectIterator<UUserExperienceComponent> It; It; ++It)
			{
				UUserExperienceComponent* Component = *It;
				Component->RegisterReporter(this);

				if (Component->GetOwner() && Component->HasBegunPlay() && Component->GetOwner()->GetWorld() == World)
				{
					if (Component->UpdateRate.Num() == 0)
					{
						// Only consider VL valid once we have at least one measurement from all sources
						bValidResult = false;
						break;
					}
					UpdateTimeDeltaMS += Component->CalculateAverageUpdateTimeDelta();
					UpdateTimeDeltaCount++;
				}
			}
			if (!bValidResult)
			{
				// A result of zero is used to indicate results aren't ready yet
				UpdateTimeDeltaMS = 0.f;
			}
			UpdateTimeDeltaMS /= UpdateTimeDeltaCount + 0.00001f; 
		}

		if (const UGDKTestGymsGameInstance* GameInstance = Cast<UGDKTestGymsGameInstance>(World->GetGameInstance()))
		{
			const UNFRConstants* Constants = UNFRConstants::Get(World);
			check(Constants);
			if (Constants->ClientFPSMetricDelay.HasTimerGoneOff() && GameInstance->GetAveragedFPS() < Constants->GetMinClientFPS())
			{
				bFrameRateValid = false;
			}
		}
		ServerReportedMetrics(RoundTripTimeMS, UpdateTimeDeltaMS, bFrameRateValid);
	}
}

void UUserExperienceReporter::ServerReportedMetrics_Implementation(float RTTMS, float UpdateTimeDeltaMS, bool bInFrameRateValid)
{
	ServerRTTMS = RTTMS;
	ServerUpdateTimeDeltaMS = UpdateTimeDeltaMS;
	if (!bInFrameRateValid) // Only change from valid to invalid
	{
		bFrameRateValid = bInFrameRateValid;
	}
}

void UUserExperienceReporter::OnClientOwnershipGained()
{
	Super::OnClientOwnershipGained();
	FTimerHandle Timer;
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &UUserExperienceReporter::ReportMetrics);
	GetWorld()->GetTimerManager().SetTimer(Timer, Delegate, 1.0f, true);
}
