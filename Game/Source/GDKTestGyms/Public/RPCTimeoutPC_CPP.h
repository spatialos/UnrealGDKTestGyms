// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RPCTimeoutPC_CPP.generated.h"

/**
 * 
 */
UCLASS()
class GDKTESTGYMS_API ARPCTimeoutPC_CPP : public APlayerController
{
	GENERATED_BODY()

	public:
	ARPCTimeoutPC_CPP();

	private:
	void OnSetMaterial(UMaterial* PlayerMaterial);

	void OnPossess(APawn* InPawn) override;

	void CheckMaterialLoaded();

	void HasValidCharacter();

	FTimerHandle MaterialTimerHandle;




	
	
};

