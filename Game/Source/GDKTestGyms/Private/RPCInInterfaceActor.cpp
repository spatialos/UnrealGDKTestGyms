// Fill out your copyright notice in the Description page of Project Settings.


#include "RPCInInterfaceActor.h"

// Sets default values
ARPCInInterfaceActor::ARPCInInterfaceActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARPCInInterfaceActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARPCInInterfaceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARPCInInterfaceActor::CallRPCInInterface()
{
	Execute_RPCInInterface(this, this);
}