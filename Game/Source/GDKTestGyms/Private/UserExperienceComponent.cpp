// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "UserExperienceComponent.h"
#include "UserExperienceReporter.h"

#include "Net/UnrealNetwork.h"
//#include "EngineClasses/SpatialNetDriver.h"
//#include "Utils/SpatialMetrics.h"

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

void UUserExperienceComponent::BeginDestroy()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(RoundTripTimer);
		TimerManager.ClearTimer(PositionCheckTimer);
	}

	Super::BeginDestroy();
}

void UUserExperienceComponent::StartRoundtrip()
{
	if (OpenRPCs.Contains(RequestKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("UserExperience failure: Previous round trip didn't complete in time (%d)."), RequestKey);
		EndRoundtrip(RequestKey);
	}
	int32 NewRPC = ++RequestKey;
	OpenRPCs.Add(NewRPC, FDateTime::Now().GetTicks());
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

	if (Reporter.IsValid())
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

// void UUserExperienceComponent::OnClientOwnershipGained()
// {
// 	Super::OnClientOwnershipGained();
// 
// 	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
// 	TimerManager.SetTimer(RoundTripTimer, this, &UUserExperienceComponent::StartRoundtrip, 1.0f, true);
// 	TimerManager.SetTimer(PositionCheckTimer, this, &UUserExperienceComponent::CheckPosition, 10.0f, true, 10.0f);
// 	PreviousPos = GetOwner()->GetActorLocation();
// }

float UUserExperienceComponent::CalculateAverageUpdateTimeDelta() const
{
	const float NCDSquared = GetOwner()->NetCullDistanceSquared;

	float Avg = 0.0f;
	for (int i = 0; i < UpdateRate.Num(); i++)
	{
		const UpdateInfo& UR = UpdateRate[i];
		Avg += UpdateRate[i].DeltaTime;
	}
	Avg /= static_cast<float>(UpdateRate.Num()) + 0.00001f;
	return Avg;
}

void UUserExperienceComponent::CheckPosition()
{
	FVector CurrPos = GetOwner()->GetActorLocation();
	if (PreviousPos == CurrPos)
	{
		UE_LOG(LogTemp, Warning, TEXT("UserExperience failure: Client position not updating (%s). Vel %s"), *PreviousPos.ToString(), *GetOwner()->GetVelocity().ToString());
	}
	PreviousPos = CurrPos;
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

