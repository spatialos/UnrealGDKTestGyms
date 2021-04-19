// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CustomPersistenceComponent.h"
#include <WorkerSDK/improbable/c_worker.h>
#include "ResourceNodePersistenceComponent.generated.h"

const Worker_ComponentId USER_PERSISTENCE_COMPONENT_ID = 9940;

USTRUCT(BlueprintType)
struct GDKTESTGYMS_API FResourceNodePersistenceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bDepleted = false;
};

UCLASS( ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UResourceNodePersistenceComponent : public UCustomPersistenceComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UResourceNodePersistenceComponent();

	virtual Worker_ComponentId GetComponentId() const override { return USER_PERSISTENCE_COMPONENT_ID; }
	virtual void GetAddComponentData(SpatialGDK::ComponentData& Data) override;
	virtual void GetComponentUpdate(SpatialGDK::ComponentUpdate& Update) override;
	virtual void OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data) override;

	UFUNCTION(BlueprintImplementableEvent)
	void GetBlueprintUpdateData(FResourceNodePersistenceData& Data);

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyBlueprintPersistenceData(const FResourceNodePersistenceData& Data);
};
