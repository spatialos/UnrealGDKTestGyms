// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsyncActorSpawner.generated.h"

UCLASS()
class GDKTESTGYMS_API AAsyncActorSpawner : public AActor
{
	GENERATED_BODY()

public:
	AAsyncActorSpawner();

protected:
	virtual void BeginPlay() override;

};
