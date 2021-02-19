// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "TestMaps/GeneratedTestMap.h"
#include "SpatialSingleWorkerTestGymMap.generated.h"

/**
 * This map is a simple map with a single server worker.
 */
UCLASS()
class USpatialSingleWorkerTestGymMap : public UGeneratedTestMap
{
	GENERATED_BODY()

public:
	USpatialSingleWorkerTestGymMap();

protected:
	virtual void CreateCustomContentForMap() override;
};
