// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceReporter.h"

#include "Net/UnrealNetwork.h"
#include "SpatialNetDriver.h"
#include "UserExperienceComponent.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogUserExperienceReporter);

namespace
{
	float CalculateAverage(const TArray<float>& Array) // In ascending order 
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
}

void UUserExperienceReporter::ReportMetrics()
{
	UUserExperienceComponent* UXComp = Cast<UUserExperienceComponent>(GetOwner()->GetComponentByClass(TSubclassOf<UUserExperienceComponent>()));
	if (UXComp)
	{
		float RTT = 0.0f;
		float ViewLateness = 0.0f;

		// Round trip
		if (UXComp->RoundTripTime.Num() == UUserExperienceComponent::NumWindowSamples) // Only start reporting once window is filled.
		{
			RTT = CalculateAverage(UXComp->RoundTripTime);
		}
		// Lateness
		{
			int32 ViewLatenessCount = 0;
			for (TObjectIterator<UUserExperienceComponent> It; It; ++It)
			{
				UUserExperienceComponent* Component = *It;
				if (Component->GetOwner() && Component->GetOwner()->GetWorld() == GetWorld())
				{
					ViewLateness += CalculateAverage(Component->UpdateRate);
					ViewLatenessCount++;
				}
			}
			ViewLateness /= ViewLatenessCount + 0.00001f; 
		}

		RTT *= 1000.0f; // ms->s
		ViewLateness *= 1000.0f; // ms->s

		ServerReportedMetrics(RTT, ViewLateness); 
	}
}

void UUserExperienceReporter::ServerReportedMetrics_Implementation(float RTT, float ViewLateness)
{
	ServerRTT = RTT;
	ServerViewLateness = ViewLateness;
}

void UUserExperienceReporter::OnClientOwnershipGained()
{
	Super::OnClientOwnershipGained();
	FTimerHandle Timer;
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &UUserExperienceReporter::ReportMetrics);
	GetWorld()->GetTimerManager().SetTimer(Timer, Delegate, 1.0f, true);
}