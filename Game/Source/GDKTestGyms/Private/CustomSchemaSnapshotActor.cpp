// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomSchemaSnapshotActor.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Net/UnrealNetwork.h"
#include "SpatialView/EntityView.h"
#include "SpatialView/ViewCoordinator.h"
#include "Utils/SpatialStatics.h"

const Worker_ComponentId PERSISTENCE_COMPONENT_ID = 9950;
const Schema_FieldId DEPLETED_FIELD_ID = 1;

// Sets default values
ACustomSchemaSnapshotActor::ACustomSchemaSnapshotActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bDepleted = false;

	PersistenceComponent = CreateDefaultSubobject<UResourceNodePersistenceComponent>(TEXT("PersistenceComponent"));
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

void ACustomSchemaSnapshotActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(ACustomSchemaSnapshotActor, bDepleted);
}
