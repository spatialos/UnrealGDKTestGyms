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

	UFUNCTION(BlueprintCallable, Category = "Maze Generation")
	static void GenerateMazeWalls(float Length, float Width, int Rows, int Cols, class UInstancedStaticMeshComponent* WallsComponent, float WallChance, int Seed);
  
};

