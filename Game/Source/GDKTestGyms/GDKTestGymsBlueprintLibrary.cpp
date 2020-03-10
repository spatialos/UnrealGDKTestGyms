// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsBlueprintLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Console.h"

void UGDKTestGymsBlueprintLibrary::SetNetAllowAsyncLoad(bool bAllow)
{
  static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowAsyncLoading"));
  if (CVar)
  {
    CVar->Set(bAllow);
  }
}

void UGDKTestGymsBlueprintLibrary::GenerateMazeWalls(float Length, float Width, int Rows, int Cols, UInstancedStaticMeshComponent* WallsComponent, float WallChance, int Seed)
{
	UE_LOG(LogTemp, Log, TEXT("HELLO"));
}