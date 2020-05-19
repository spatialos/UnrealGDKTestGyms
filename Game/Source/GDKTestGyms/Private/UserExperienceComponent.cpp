// Fill out your copyright notice in the Description page of Project Settings.


#include "UserExperienceComponent.h"

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
	ElapsedSeconds = 0.0f;
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
	OpenRPCs.Add(NewRPC, ElapsedSeconds);
	ServerRTT(NewRPC);
}

void UUserExperienceComponent::EndRoundtrip(int32 Key)
{
	if (float* StartTime = OpenRPCs.Find(Key))
	{
		float Diff = ElapsedSeconds - *StartTime;
		RoundTripTime.Push(Diff);
		if (RoundTripTime.Num() > NumWindowSamples)
		{
			RoundTripTime.RemoveAt(0);
		}
		OpenRPCs.Remove(Key);
	}
}

void UUserExperienceComponent::OnRep_ClientTime(float OldValueSeconds)
{
	if (!bHadClientTimeRep) // Ignore initial replication
	{
		bHadClientTimeRep = true;
		return;
	}

	float Diff = ClientTimeSeconds - OldValueSeconds;
	UpdateRate.Push(Diff);
	if (UpdateRate.Num() > NumWindowSamples)
	{
		UpdateRate.RemoveAt(0);
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

// Called every frame
void UUserExperienceComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	
	ElapsedSeconds += DeltaSeconds;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor->HasAuthority())
	{
		// Update replicated time to clients
		ClientTimeSeconds = ElapsedSeconds;
		OwnerActor->ForceNetUpdate();
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

	DOREPLIFETIME(UUserExperienceComponent, ClientTimeSeconds);
}

