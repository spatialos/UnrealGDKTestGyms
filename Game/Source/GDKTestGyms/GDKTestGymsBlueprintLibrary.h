// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GDKTestGymsBlueprintLibrary.generated.h"

UCLASS()
class UGDKTestGymsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

  UFUNCTION(BlueprintCallable, Category = "GDKTests")
  static void SetNetAllowAsyncLoad(bool bAllow);
};

