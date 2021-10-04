// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GDKTestGyms/GDKTestGymsCharacter.h"
#include "RPCTimeoutCharacter.generated.h"

UCLASS()
class GDKTESTGYMS_API ARPCTimeoutCharacter : public AGDKTestGymsCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARPCTimeoutCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
