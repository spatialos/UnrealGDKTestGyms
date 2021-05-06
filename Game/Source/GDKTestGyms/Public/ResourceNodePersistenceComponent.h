// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "SchemaPersistence/SchemaPersistenceComponent.h"
#include <WorkerSDK/improbable/c_worker.h>
#include "ResourceNodePersistenceComponent.generated.h"

const Worker_ComponentId GYMS_RESOURCE_PERSISTENCE_COMPONENT_ID = 1002;

USTRUCT(BlueprintType)
struct GDKTESTGYMS_API FResourceNodePersistenceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bDepleted = false;
};

UCLASS( ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UResourceNodePersistenceComponent : public USchemaPersistenceComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UResourceNodePersistenceComponent();

	virtual Worker_ComponentId GetComponentId() const override { return GYMS_RESOURCE_PERSISTENCE_COMPONENT_ID; }
	virtual void GetAddComponentData(SpatialGDK::ComponentData& Data) override;
	virtual void GetComponentUpdate(SpatialGDK::ComponentUpdate& Update) override;
	virtual void OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data) override;

	UFUNCTION(BlueprintImplementableEvent)
	void GetBlueprintUpdateData(FResourceNodePersistenceData& Data);

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyBlueprintPersistenceData(const FResourceNodePersistenceData& Data);
};
