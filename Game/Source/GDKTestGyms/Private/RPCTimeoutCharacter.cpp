// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutCharacter.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARPCTimeoutCharacter::ARPCTimeoutCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	const TCHAR* DefaultSkeletalMeshPathString = TEXT("SkeletalMesh'/Engine/EngineMeshes/SkeletalCube.SkeletalCube'");
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>DefaultSkeletalMeshFinder(DefaultSkeletalMeshPathString);
	USkeletalMesh* DefaultSkeletalMesh = DefaultSkeletalMeshFinder.Object;
	checkf(IsValid(DefaultSkeletalMesh), TEXT("Could not find failed material asset %ls"), DefaultSkeletalMeshPathString);

	GetMesh()->SetSkeletalMesh(DefaultSkeletalMesh);
}
