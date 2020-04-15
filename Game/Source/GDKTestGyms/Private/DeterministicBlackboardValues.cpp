// Fill out your copyright notice in the Description page of Project Settings.


#include "DeterministicBlackboardValues.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY(LogDeterministicBlackboardValues);

void UDeterministicBlackboardValues::ApplyValues() // Repeats until the Component is added 
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn->GetController();

	AAIController* AIController = Cast<AAIController>(Controller);
	if (AIController == nullptr)
	{
		AIController = Cast<AAIController>(Pawn);
	}

	if (AIController)
	{
		UBlackboardComponent* Blackboard = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		checkf(Blackboard, TEXT("AI Controller did not have a blackboard %s"), *Controller->GetPawn()->GetName());

		FVector WorldLocation = ((AActor*)Controller)->GetActorLocation();

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

		constexpr float Tolerance = 100.0f; // A tolerance to allow the point to snap to the nav mesh surfaces.

		FNavLocation LocA;
		{
			FVector LocalRandomPoint = WorldLocation + BlackboardValues.TargetAValue;
			bool Result = NavSys->GetRandomPointInNavigableRadius(LocalRandomPoint, Tolerance, LocA);
			checkf(Result, TEXT("Could not find a point in nav mesh at %s"), *LocalRandomPoint.ToString());
		}

		FNavLocation LocB;
		{
			FVector LocalRandomPoint = WorldLocation + BlackboardValues.TargetBValue;
			bool Result = NavSys->GetRandomPointInNavigableRadius(LocalRandomPoint, Tolerance, LocB);
			checkf(Result, TEXT("Could not find a point in nav mesh at %s"), *LocalRandomPoint.ToString());
		}
		
		Blackboard->SetValueAsVector(BlackboardValues.TargetAName, LocA);
		Blackboard->SetValueAsVector(BlackboardValues.TargetBName, LocB);

		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("Setting points to run between as %s and %s for AI controller %s"), *LocA.Location.ToString(), *LocB.Location.ToString(), *Controller->GetName());
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	else
	{
#if !WITH_EDITOR
		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("Could not find an AI controller."));
#endif
	}
}
void UDeterministicBlackboardValues::ClientSetBlackboardAILocations_Implementation(const FBlackboardValues& InBlackboardValues)
{
	BlackboardValues = InBlackboardValues;

	// Sim-players do not spawn their AI controllers immediately, this timer will pump 
	// until the AI controller + blackboard is spawned and then set the values.
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UDeterministicBlackboardValues::ApplyValues, 1.0f, true);
}