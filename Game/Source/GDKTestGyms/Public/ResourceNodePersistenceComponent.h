// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EngineClasses/Components/CustomPersistenceComponent.h"
#include "ResourceNodePersistenceComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UResourceNodePersistenceComponent : public UCustomPersistenceComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UResourceNodePersistenceComponent();

	virtual Worker_ComponentId GetComponentId() const override { return (Worker_ComponentId)9950; }
	virtual void GetAddComponentData(SpatialGDK::ComponentData& Data) override;
	virtual void GetComponentUpdate(SpatialGDK::ComponentUpdate& Update) override;
	virtual void OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data) override;
	
	UPROPERTY(BlueprintReadWrite)
	bool bDepleted;
};


