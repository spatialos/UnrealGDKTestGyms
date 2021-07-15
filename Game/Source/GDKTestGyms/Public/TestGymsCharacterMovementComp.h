// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TestGymsCharacterMovementComp.generated.h"

class AGDKTestGymsCharacter;

UCLASS()
class GDKTESTGYMS_API UTestGymsCharacterMovementComp : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void PostLoad() override;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
	virtual void ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration) override;

	void ClientAuthServerMove_Implementation(const FVector_NetQuantize100& ClientLocation, const FVector_NetQuantize10& ClientVelocity, const uint32 PackedPitchYaw);
	
	bool bClientAuthMovement = false;
	int32 ClientMoveFrequency = 10; // Send client position updates to server at this frequency
	float LastMoveWorldTime = 0.f;

	UPROPERTY()
	AGDKTestGymsCharacter* TestGymsCharacterOwner;
};
