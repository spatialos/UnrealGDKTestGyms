// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RPCTimeoutPC.generated.h"

/**
* 
*/
UCLASS()
class GDKTESTGYMS_API ARPCTimeoutPC: public APlayerController
{
	GENERATED_BODY()

	public:
	ARPCTimeoutPC();
	
	private:
	UFUNCTION(Client,Reliable)
	void OnSetMaterial(UMaterial* PlayerMaterial);
	
	UFUNCTION(Client,Reliable)
	void CheckMaterialLoaded();

	virtual void OnPossess(APawn* InPawn) override;
	
	void CheckValidCharacter();

	void SetMaterialAfterDelay();

	UPROPERTY()
	UMaterial* FailedMaterialAsset;

	TSoftObjectPtr<UMaterial> SoftMaterialPtr;
	
	FSoftObjectPath SoftMaterialPath;

	FTimerHandle HasValidCharacterTimer;
	
	FTimerHandle MaterialSetDelay;
};

