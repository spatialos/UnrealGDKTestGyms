// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GDKTestGymsBlueprintLibrary.h"
#include "Engine/Console.h"

void UGDKTestGymsBlueprintLibrary::SetNetAllowAsyncLoad(bool bAllow)
{
  static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowAsyncLoading"));
  if (CVar)
  {
    CVar->Set(bAllow);
  }
}