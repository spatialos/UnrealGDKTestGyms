// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BenchmarkNPCCharacter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBenchmarkNPCCharacter, Log, All);

UCLASS()
class GDKTESTGYMS_API ABenchmarkNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnAuthorityGained() override;
	virtual void OnAuthorityLost() override;

};
