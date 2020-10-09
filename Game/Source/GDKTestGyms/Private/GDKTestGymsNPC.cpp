// Fill out your copyright notice in the Description page of Project Settings.


#include "GDKTestGymsNPC.h"

// Sets default values
AGDKTestGymsNPC::AGDKTestGymsNPC()
{}

void AGDKTestGymsNPC::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	SetReplicateMovement(false);
	Super::PreReplication(ChangedPropertyTracker);
	SetReplicateMovement(true);
}
