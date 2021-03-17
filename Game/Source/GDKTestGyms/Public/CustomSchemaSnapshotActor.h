// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Schema/CustomPersistence.h"
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	void FillSchemaData(Schema_Object& ComponentObject) const;
	
	UPROPERTY(ReplicatedUsing=OnRep_bDepleted, BlueprintReadWrite)
	bool bDepleted;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_bDepleted();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnLoadedFromSnapshot"))
	void OnPostInitComponentsSetupBlueprintFromSnapshot();
};
