// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "MaterialArray_CPP.h"

// Sets default values
AMaterialArray_CPP::AMaterialArray_CPP()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

