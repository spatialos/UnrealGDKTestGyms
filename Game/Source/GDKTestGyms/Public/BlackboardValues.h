// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlackboardValues.generated.h"

USTRUCT()
struct GDKTESTGYMS_API FBlackboardValues 
{
	GENERATED_BODY()

	UPROPERTY()
	FVector TargetAValue = FVector(EForceInit::ForceInit); // Points to run between
	UPROPERTY()
	FVector TargetBValue = FVector(EForceInit::ForceInit);
	UPROPERTY()
	bool TargetStateIsA = false; // Keep track of current goal target, to continue towards it after handover. True for A, false for B
	UPROPERTY()
	bool bInitialised = false;

	static FName TargetAName; // Bindings to the AI blackboard values
	static FName TargetBName;
	static FName TargetStateIsAName;
	static FName InitialisedName;
};
