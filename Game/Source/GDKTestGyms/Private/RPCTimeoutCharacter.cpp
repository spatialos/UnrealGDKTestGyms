// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutCharacter.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARPCTimeoutCharacter::ARPCTimeoutCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>DefaultSkeletalMeshFinder(TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube.SkeletalCube'"));
	USkeletalMesh* DefaultSkeletalMesh = DefaultSkeletalMeshFinder.Object;

	GetMesh()->SetSkeletalMesh(DefaultSkeletalMesh);
	
}