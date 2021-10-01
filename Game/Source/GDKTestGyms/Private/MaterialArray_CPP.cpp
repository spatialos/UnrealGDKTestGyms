// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "MaterialArray_CPP.h"

// Sets default values
AMaterialArray_CPP::AMaterialArray_CPP()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Material1 = CreateDefaultSubobject<UMaterial>("Material1");
	
	Material1 = TSoftObjectPtr<UMaterial>(FSoftObjectPath(TEXT("Material'/Game/SoftReferenceTest/Green1.Green1'")));
	Material2 = TSoftObjectPtr<UMaterial>(FSoftObjectPath(TEXT("Material'/Game/SoftReferenceTest/Green2.Green2'")));
	Material3 = TSoftObjectPtr<UMaterial>(FSoftObjectPath(TEXT("Material'/Game/SoftReferenceTest/Green3.Green3'")));
	Material4 = TSoftObjectPtr<UMaterial>(FSoftObjectPath(TEXT("Material'/Game/SoftReferenceTest/Green4.Green4'")));
}

// Called when the game starts or when spawned
void AMaterialArray_CPP::BeginPlay()
{
	Super::BeginPlay();
	
	if(HasAuthority())
	{
		if(TestGrid)
		{
			TestGrid->SoftMaterial = Material1;
			TestGrid->SoftMaterialArray.Add(Material2);
			TestGrid->Material = Material3.LoadSynchronous();
			TestGrid->MaterialArray.Add(Material4.LoadSynchronous());
		}
	}
}

// Called every frame
void AMaterialArray_CPP::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

