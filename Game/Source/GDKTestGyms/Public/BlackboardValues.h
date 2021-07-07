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
	FVector TargetAValue; // Points to run between
	UPROPERTY()
	FVector TargetBValue;
	UPROPERTY()
	bool TargetStateIsA; // Keep track of current goal target, to continue towards it after handover. True for A, false for B

	static FName TargetAName; // Bindings to the AI blackboard values
	static FName TargetBName;
	static FName TargetStateIsAName;
};
