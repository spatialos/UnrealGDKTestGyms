// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialLockingActorComponent.h"

#include "Containers/UnrealString.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "Net/UnrealNetwork.h"

#include "LoadBalancing/AbstractLockingPolicy.h"

USpatialLockingComponent::USpatialLockingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
#if ENGINE_MINOR_VERSION <= 23
	bReplicates = true;
#else
	SetIsReplicatedByDefault(true);
#endif
}

void USpatialLockingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpatialLockingComponent, bIsLocked);
}

int64 USpatialLockingComponent::AcquireLock()
{
	++LockCounter;
	const auto LockToken = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver())->LockingPolicy->AcquireLock(
		GetOwner(),
		FString::Printf(TEXT("[DEBUG STRING] this is lock attempt %d for actor %s :)"), LockCounter, *GetOwner()->GetName()));
	check(LockToken != SpatialConstants::INVALID_ACTOR_LOCK_TOKEN);
	UE_LOG(LogTemp, Warning, TEXT("[SpatialLockingComponent] Called AcquireLock. Got token: %d"), LockToken);
	bIsLocked = true;
	return LockToken;
}

bool USpatialLockingComponent::IsLocked() const
{
	// Servers query the locking policy. Clients check if the server has replicated that it's locked.
	const auto* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver->IsServer())
	{
		return  NetDriver->LockingPolicy->IsLocked(GetOwner());
	}
	return bIsLocked;
}

void USpatialLockingComponent::ReleaseLock(int64 LockToken)
{
	UE_LOG(LogTemp, Warning, TEXT("[SpatialLockingComponent] Called ReleaseLock for token: %d"), LockToken);
	AActor* Owner = GetOwner();
	Cast<USpatialNetDriver>(GetWorld()->GetNetDriver())->LockingPolicy->ReleaseLock(LockToken);
	bIsLocked = IsLocked();
}
