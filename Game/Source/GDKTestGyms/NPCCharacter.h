// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "NPCCharacter.generated.h"

UCLASS()
class GDKTESTGYMS_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANPCCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Used for timing when the fake client will change the direction it is moving
	FTimerHandle TimerHandler;

	// Called to change the direction the character is running to the right
	void TurnRight();

	int DirectionIndex = 0;
	const FVector Directions[4] = { FVector(1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, -1, 0) };

	// Magnitude multiplier used on movement impulses
	float Speed = 0.5f;
};
