// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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
	virtual void Tick(float DeltaTime) override;
	
private:
	UFUNCTION(Client,Reliable)
	void OnSetMaterial(UMaterial* PlayerMaterial);
	
	UFUNCTION(Client,Reliable)
	void CheckMaterialLoaded();
	
	void OnPossess(APawn* InPawn) override;
	
	void HasValidCharacter();

	void SetMaterialAfterDelay();

	UPROPERTY()
	UMaterial* FailedMaterialAsset;

	TSoftObjectPtr<UMaterial> SoftMaterialPtr;
	
	FSoftObjectPath SoftMaterialPath;

	FTimerHandle HasValidCharacterTimer;
	
	FTimerHandle MaterialSetDelay;
};

