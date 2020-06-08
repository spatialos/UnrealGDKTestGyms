// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceReporter.h"

#include "Net/UnrealNetwork.h"
#include "SpatialNetDriver.h"
#include "UserExperienceComponent.h"
#include "Utils/SpatialMetrics.h"
#include "GDKTestGymsGameInstance.h"
#include "NFRConstants.h"

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
	ServerRTT = 0.0f;
	ServerViewLateness = 0.0f;
}

void UUserExperienceReporter::ReportMetrics()
{
	UUserExperienceComponent* UXComp = Cast<UUserExperienceComponent>(GetOwner()->FindComponentByClass(UUserExperienceComponent::StaticClass()));
	if (UXComp)
	{
		float RoundTripTimeMS = 0.0f;
		float ViewLatenessMS = 0.0f;

		// Round trip
		if (UXComp->RoundTripTime.Num() == UUserExperienceComponent::NumWindowSamples) // Only start reporting once window is filled.
		{
			RoundTripTimeMS = CalculateAverage(UXComp->RoundTripTime);
		}
		// Lateness
		{
			int32 ViewLatenessCount = 0;
			for (TObjectIterator<UUserExperienceComponent> It; It; ++It)
			{
				UUserExperienceComponent* Component = *It;
				if (Component->GetOwner() && Component->GetOwner()->GetWorld() == GetWorld() && Component->UpdateRate.Num() == UUserExperienceComponent::NumWindowSamples)
				{
					ViewLatenessMS += CalculateAverage(Component->UpdateRate);
					ViewLatenessCount++;
				}
			}
			ViewLatenessMS /= ViewLatenessCount + 0.00001f; 
		}

		if (const UGDKTestGymsGameInstance* GameInstance = Cast<UGDKTestGymsGameInstance>(GetWorld()->GetGameInstance()))
		{
			const UNFRConstants* Constants = UNFRConstants::Get(GetWorld());
			check(Constants);
			if (Constants->SamplesForFPSValid() && GameInstance->GetAveragedFPS() < Constants->GetMinClientFPS())
			{
				bFrameRateValid = false;
			}
		}
		ServerReportedMetrics(RoundTripTimeMS, ViewLatenessMS, bFrameRateValid);
	}
}

void UUserExperienceReporter::ServerReportedMetrics_Implementation(float RTTSeconds, float ViewLatenessSeconds, bool bInFrameRateValid)
{
	ServerRTT = RTTSeconds;
	ServerViewLateness = ViewLatenessSeconds;
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