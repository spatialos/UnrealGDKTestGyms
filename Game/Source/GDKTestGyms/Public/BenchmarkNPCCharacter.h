// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BenchmarkNPCCharacter.generated.h"

UCLASS()
class GDKTESTGYMS_API ABenchmarkNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	virtual void OnAuthorityGained() override;
	virtual void OnAuthorityLost() override;

};
