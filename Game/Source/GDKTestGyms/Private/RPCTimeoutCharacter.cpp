// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutCharacter.h"

// Sets default values
ARPCTimeoutCharacter::ARPCTimeoutCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>DefaultSkeletalMeshFinder(TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube.SkeletalCube'"));
	USkeletalMesh* DefaultSkeletalMesh = DefaultSkeletalMeshFinder.Object;

	GetMesh()->SetSkeletalMesh(DefaultSkeletalMesh);
	
}

// Called when the game starts or when spawned
void ARPCTimeoutCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARPCTimeoutCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ARPCTimeoutCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

