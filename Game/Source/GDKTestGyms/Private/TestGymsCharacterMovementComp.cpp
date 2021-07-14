// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "TestGymsCharacterMovementComp.h"
#include "../GDKTestGymsCharacter.h"

void UTestGymsCharacterMovementComp::BeginPlay()
{
	Super::BeginPlay();

	if (bClientAuthMovement)
	{
		if (ensure(TestGymsCharacterOwner != nullptr))
		{
			// When running client auth movement, disable the local actor's movement replication so that 
			// ServerMove RPCs won't be sent. We don't do this on the server though, because we still
			// want the ReplicatedMovement struct to be replicated to others.
			if (TestGymsCharacterOwner->GetLocalRole() == ROLE_AutonomousProxy)
			{
				UE_LOG(LogTemp, Log, TEXT("Enabling client auth movement, disabling movement replication locally."));
				TestGymsCharacterOwner->SetReplicateMovement(false);
			}
		}
	}
}

void UTestGymsCharacterMovementComp::PostLoad()
{
	Super::PostLoad();
	TestGymsCharacterOwner = Cast<AGDKTestGymsCharacter>(CharacterOwner);
}

void UTestGymsCharacterMovementComp::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	Super::SetUpdatedComponent(NewUpdatedComponent);
	TestGymsCharacterOwner = Cast<AGDKTestGymsCharacter>(CharacterOwner);
}

void UTestGymsCharacterMovementComp::ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration)
{
	Super::ReplicateMoveToServer(DeltaTime, NewAcceleration);

	if (!bClientAuthMovement)
	{
		return;
	}

	const float CurrentWorldTime = GetWorld()->GetTimeSeconds();

	if (CurrentWorldTime > (LastMoveWorldTime + (1.f/ClientMoveFrequency)))
	{
		FVector ClientLocation = TestGymsCharacterOwner->GetActorLocation();
		FVector ClientVelocity = Velocity;
		uint32 PackedPitchYaw = UCharacterMovementComponent::PackYawAndPitchTo32(TestGymsCharacterOwner->GetActorRotation().Yaw, TestGymsCharacterOwner->GetActorRotation().Pitch);

		TestGymsCharacterOwner->ClientAuthServerMove(ClientLocation, ClientVelocity, PackedPitchYaw);
		LastMoveWorldTime = CurrentWorldTime;
	}
}

void UTestGymsCharacterMovementComp::ClientAuthServerMove_Implementation(const FVector_NetQuantize100& ClientLocation, const FVector_NetQuantize10& ClientVelocity, const uint32 PackedPitchYaw)
{
	// View components
	const uint16 ShortPitch = (PackedPitchYaw & 65535);
	const uint16 ShortYaw = (PackedPitchYaw >> 16);
	FRotator NewRotation(FRotator::DecompressAxisFromShort(ShortPitch), FRotator::DecompressAxisFromShort(ShortYaw), 0.f);

	TestGymsCharacterOwner->SetActorLocation(ClientLocation);
	TestGymsCharacterOwner->SetActorRotation(NewRotation);
	Velocity = ClientVelocity;
}
