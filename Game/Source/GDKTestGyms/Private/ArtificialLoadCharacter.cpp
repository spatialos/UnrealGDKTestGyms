// Fill out your copyright notice in the Description page of Project Settings.


#include "ArtificialLoadCharacter.h"

// Required for artificial loads (NFRs until AI is implemented) - FGenericPlatformProcess::Sleep is not available on Windows.
#include <chrono>
#include <thread>

// Sets default values
AArtificialLoadCharacter::AArtificialLoadCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AArtificialLoadCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AArtificialLoadCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ArtificialWorkload > FLT_EPSILON)
	{
		// FGenericPlatformProcess::Sleep is not available on Windows?
		std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(ArtificialWorkload));
	}
}

// Called to bind functionality to input
void AArtificialLoadCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

