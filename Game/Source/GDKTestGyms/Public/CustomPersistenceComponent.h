// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SpatialView/ComponentData.h"
#include "SpatialView/ComponentUpdate.h"

#include "CustomPersistenceComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class GDKTESTGYMS_API UCustomPersistenceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCustomPersistenceComponent();

	virtual Worker_ComponentId GetComponentId() const { return (Worker_ComponentId)0; } // todo use invalid component id constant

	// Internal handling of adding/updating/applying the persistence component data
	virtual void PostReplication() override;
	virtual void OnAuthorityGained() override;

protected:
	// User callbacks to provide data
	virtual void GetAddComponentData(SpatialGDK::ComponentData& Data);
	virtual void GetComponentUpdate(SpatialGDK::ComponentUpdate& Update);
	virtual void OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data);
};