// Fill out your copyright notice in the Description page of Project Settings.


#include "NPCCharacter.h"

// Sets default values
ANPCCharacter::ANPCCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(TimerHandler, this, &ANPCCharacter::TurnRight, 1.0f, true, 0.0f);
}

// Called every frame
void ANPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AddMovementInput(Directions[DirectionIndex] * Speed);
}

// Called to bind functionality to input
void ANPCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ANPCCharacter::TurnRight()
{
	DirectionIndex++;
	if (DirectionIndex >= 4)
	{
		DirectionIndex = 0;
	}
}
