// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RPCTimeoutPlayerController.generated.h"

/**
* 
*/
UCLASS()
class GDKTESTGYMS_API ARPCTimeoutPlayerController: public APlayerController
{
	GENERATED_BODY()

	public:
	ARPCTimeoutPlayerController();
	
	private:
	UFUNCTION(Client, Reliable)
	void OnSetMaterial(UMaterial* PlayerMaterial);
	
	UFUNCTION(Client, Reliable)
	void CheckMaterialLoaded();

	virtual void OnPossess(APawn* InPawn) override;
	
	void CheckValidCharacter();

	void SetMaterialAfterDelay();

	UPROPERTY()
	UMaterial* FailedMaterialAsset;

	TSoftObjectPtr<UMaterial> SoftMaterialPtr;
	
	FTimerHandle HasValidCharacterTimer;
	
	FTimerHandle MaterialSetDelay;
};

