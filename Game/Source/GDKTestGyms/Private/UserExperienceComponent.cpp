// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"
#include "UserExperienceReporter.h"

#include "Net/UnrealNetwork.h"
#include "SpatialNetDriver.h"
#include "Utils/SpatialMetrics.h"

DEFINE_LOG_CATEGORY(LogUserExperienceComponent);

// Sets default values for this component's properties
UUserExperienceComponent::UUserExperienceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UUserExperienceComponent::InitializeComponent()
{
	UActorComponent::InitializeComponent();
	RequestKey = 0;
	bHadClientTimeRep = false;
	SetIsReplicated(true);
}

void UUserExperienceComponent::StartRoundtrip()
{
	if (OpenRPCs.Contains(RequestKey))
	{
		EndRoundtrip(RequestKey);
	}
	int32 NewRPC = ++RequestKey;
	OpenRPCs.Add(NewRPC, FDateTime::Now().GetTicks() );
	ServerRTT(NewRPC);
}

void UUserExperienceComponent::EndRoundtrip(int32 Key)
{
	if (int64* StartTime = OpenRPCs.Find(Key))
	{
		int64_t Diff = FDateTime::Now().GetTicks() - *StartTime;
		RoundTripTime.Push(FTimespan(Diff).GetTotalMilliseconds());
		if (RoundTripTime.Num() > NumWindowSamples)
		{
			RoundTripTime.RemoveAt(0);
		}
		OpenRPCs.Remove(Key);
	}
}

void UUserExperienceComponent::OnRep_ClientTimeTicks(int64 OldTicks)
{
	if (!bHadClientTimeRep) // Ignore initial replication
	{
		bHadClientTimeRep = true;
		return;
	}

	if (Reporter != nullptr && !Reporter->IsPendingKill())
	{
		float DeltaTime = FTimespan(ClientTimeTicks - OldTicks).GetTotalMilliseconds();
		float DistanceSq = Reporter->GetOwner()->GetSquaredDistanceTo(GetOwner());
		UpdateRate.Push({DeltaTime, DistanceSq});
		if (UpdateRate.Num() > NumWindowSamples)
		{
			UpdateRate.RemoveAt(0);
		}
	}
}

void UUserExperienceComponent::OnClientOwnershipGained()
{
	Super::OnClientOwnershipGained();
	FTimerHandle Timer;
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &UUserExperienceComponent::StartRoundtrip);
	GetWorld()->GetTimerManager().SetTimer(Timer, Delegate, 1.0f, true);
}

float UUserExperienceComponent::CalculateAverageVL() const
{
	const float NCDSquared = GetOwner()->NetCullDistanceSquared;

	float Avg = 0.0f;
	for (int i = 0; i < UpdateRate.Num(); i++)
	{
		const UpdateInfo& UR = UpdateRate[i];
		check(UR.DistanceSq < NCDSquared);
		Avg += (UpdateRate[i].DeltaTime * (1.f - UR.DistanceSq / NCDSquared));
	}
	Avg /= static_cast<float>(UpdateRate.Num()) + 0.00001f;
	return Avg;
}

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor->HasAuthority())
	{
		// Update replicated time to clients
		ClientTimeTicks = FDateTime::Now().GetTicks();
	}
}

void UUserExperienceComponent::ServerRTT_Implementation(int32 Key)
{
	ClientRTT(Key); // For round-trip
}

void UUserExperienceComponent::ClientRTT_Implementation(int32 Key)
{
	EndRoundtrip(Key);
}

void UUserExperienceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUserExperienceComponent, ClientTimeTicks);
}

