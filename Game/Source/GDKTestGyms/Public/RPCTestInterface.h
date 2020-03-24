#pragma once

#include "UObject/Interface.h"
#include "Engine/Classes/Engine/EngineTypes.h"
#include "RPCTestInterface.generated.h"

UINTERFACE(Blueprintable)
class URPCTestInterface : public UInterface
{
	GENERATED_BODY()
};

class IRPCTestInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(CrossServer, Reliable)
	virtual void RPCInInterface(UObject* EventObject);

	virtual void RPCInInterface_Implementation(UObject* EventObject)
	{
		Execute_OnRPCInInterface(EventObject);
	}

	UFUNCTION(BlueprintNativeEvent)
		void OnRPCInInterface();
};
