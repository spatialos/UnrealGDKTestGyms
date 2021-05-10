// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ResourceNodePersistenceComponent.h"
#include "GameFramework/Actor.h"
#include "CustomSchemaSnapshotActor.generated.h"

UCLASS()
class GDKTESTGYMS_API ACustomSchemaSnapshotActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACustomSchemaSnapshotActor();

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnAuthorityGained() override;
	virtual void OnAuthorityLost() override;
	virtual void OnActorReady(bool bHasAuthority) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(ReplicatedUsing=OnRep_bDepleted, BlueprintReadWrite)
	bool bDepleted;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_bDepleted();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
