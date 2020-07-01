// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "BenchmarkGymGameModeBase.h"

#include "OffloadingBenchmarkGymGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOffloadingBenchmarkGymGameMode, Log, All);

UCLASS()
class GDKTESTGYMS_API AOffloadingBenchmarkGymGameMode : public ABenchmarkGymGameModeBase
{
	GENERATED_BODY()

public:

	AOffloadingBenchmarkGymGameMode();

protected:

	virtual void BuildExpectedActorCounts() override;

private:

	TSubclassOf<AActor> NPCBPClass;
	TSubclassOf<AActor> SimulatedPlayerBPClass;
};
