// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "Improbable/SpatialEngineConstants.h"
#include "SpatialView/ComponentData.h"
#include "SpatialView/ComponentUpdate.h"

#include "SchemaPersistenceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSchemaPersistence, Warning, All);

UCLASS(meta = (BlueprintSpawnableComponent))
class USchemaPersistenceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USchemaPersistenceComponent();

	virtual Worker_ComponentId GetComponentId() const
	{
		return SpatialConstants::INVALID_ENTITY_ID;
	}

	// Internal handling of adding/updating/applying the persistence component data
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void OnActorEntityCreated(TArray<SpatialGDK::ComponentData>& OutComponentDatas);
	void OnActorReplication(TArray<SpatialGDK::ComponentUpdate>& OutComponentUpdates);
	virtual void OnAuthorityGained() override;
	virtual void OnAuthorityLost() override;

protected:
	// User callbacks to provide data
	virtual void GetAddComponentData(SpatialGDK::ComponentData& Data);
	virtual void GetComponentUpdate(SpatialGDK::ComponentUpdate& Update);
	virtual void OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data);

private:
	FDelegateHandle OnActorReplicationDelegateHandle;

	void RemoveActorReplicationDelegate();
};
