// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomSchemaSnapshotActor.h"

// Sets default values
ACustomSchemaSnapshotActor::ACustomSchemaSnapshotActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACustomSchemaSnapshotActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACustomSchemaSnapshotActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Import Custom Schema before beginplay() and after component interference
void ACustomSchemaSnapshotActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	OnPostInitComponentsSetupBlueprintFromSnapshot();
}
