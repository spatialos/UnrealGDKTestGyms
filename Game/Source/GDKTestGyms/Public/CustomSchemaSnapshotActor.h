// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomSchemaSnapshotActor.generated.h"

UCLASS()
class GDKTESTGYMS_API ACustomSchemaSnapshotActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACustomSchemaSnapshotActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnLoadedFromSnapshot"))
	void OnPostInitComponentsSetupBlueprintFromSnapshot();
};
