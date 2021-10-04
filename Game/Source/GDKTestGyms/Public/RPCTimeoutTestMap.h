// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "TestMaps/GeneratedTestMap.h"
#include "RPCTimeoutTestMap.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API URPCTimeoutTestMap : public UGeneratedTestMap
{
	GENERATED_BODY()

	public:
	URPCTimeoutTestMap();


	protected:
	virtual void CreateCustomContentForMap() override;
};

