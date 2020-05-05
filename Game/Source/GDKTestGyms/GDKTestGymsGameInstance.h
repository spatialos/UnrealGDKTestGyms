// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "SpatialGameInstance.h"
#include "GDKTestGymsGameInstance.generated.h"

UCLASS()
class GDKTESTGYMS_API UGDKTestGymsGameInstance : public USpatialGameInstance
{
	GENERATED_BODY()

public:
	UGDKTestGymsGameInstance();

	virtual void Init() override;
	//virtual TSubclassOf<AGameModeBase> OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass, const FString& MapName, const FString& Options, const FString& Portal) const;

	bool Tick(float DeltaSeconds);
	virtual void OnStart() override;
private:
	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	TSubclassOf<AGameModeBase> LatencyGameModeClass;

	float AverageFPS = 60.0f;
	float SecondsSinceFPSLog = 0.0f;
};
