// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EngineClasses/Components/CustomPersistenceComponent.h"
#include "ResourceNodePersistenceComponent.generated.h"

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

	virtual Worker_ComponentId GetComponentId() const override { return (Worker_ComponentId)9950; }
	virtual void GetAddComponentData(SpatialGDK::ComponentData& Data) override;
	virtual void GetComponentUpdate(SpatialGDK::ComponentUpdate& Update) override;
	virtual void OnPersistenceDataAvailable(const SpatialGDK::ComponentData& Data) override;

	UFUNCTION(BlueprintImplementableEvent)
	void GetBlueprintUpdateData(FResourceNodePersistenceData& Data);

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyBlueprintPersistenceData(const FResourceNodePersistenceData& Data);
};
