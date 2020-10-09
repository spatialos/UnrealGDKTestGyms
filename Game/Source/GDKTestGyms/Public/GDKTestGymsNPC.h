// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GDKTestGymsNPC.generated.h"

UCLASS()
class GDKTESTGYMS_API AGDKTestGymsNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AGDKTestGymsNPC();

public:	
	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;
};
