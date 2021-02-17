// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "TestMaps/GeneratedTestMap.h"
#include "Spatial2WorkerTestGymMap.generated.h"

/**
 * This map is a simple 2-server-worker map, where both workers see everything in the world.
 */
UCLASS()
class USpatial2WorkerTestGymMap : public UGeneratedTestMap
{
	GENERATED_BODY()

public:
	USpatial2WorkerTestGymMap();

protected:
	virtual void CreateCustomContentForMap() override;
};
