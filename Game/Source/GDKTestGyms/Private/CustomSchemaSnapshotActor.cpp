// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomSchemaSnapshotActor.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Net/UnrealNetwork.h"
#include "SpatialView/EntityView.h"
#include "SpatialView/ViewCoordinator.h"
#include "Utils/SpatialStatics.h"

// Sets default values
ACustomSchemaSnapshotActor::ACustomSchemaSnapshotActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bDepleted = false;
}

// Called when the game starts or when spawned
void ACustomSchemaSnapshotActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("entity recreation / %s / BeginPlay, auth: %d"), *GetName(), HasAuthority());
}

void ACustomSchemaSnapshotActor::OnAuthorityGained()
{
	Super::OnAuthorityGained();

	UE_LOG(LogTemp, Log, TEXT("entity recreation / %s / OnAuthorityGained"), *GetName());
}

void ACustomSchemaSnapshotActor::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	UE_LOG(LogTemp, Log, TEXT("entity recreation / %s / OnAuthorityLost"), *GetName());
}

void ACustomSchemaSnapshotActor::OnActorReady(bool bHasAuthority)
{
	Super::OnActorReady(bHasAuthority);

	UE_LOG(LogTemp, Log, TEXT("entity recreation / %s / OnActorReady"), *GetName())
}

// Called every frame
void ACustomSchemaSnapshotActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACustomSchemaSnapshotActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ACustomSchemaSnapshotActor, bDepleted);
}
