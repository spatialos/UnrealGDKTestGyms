// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "DeterministicBlackboardValues.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY(LogDeterministicBlackboardValues);

void UDeterministicBlackboardValues::ApplyValues() // Repeats until the Component is added 
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn->GetController();

	if (Controller == nullptr)
	{
#if !WITH_EDITOR
		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("Pawn controller is not set, retrying in 1 second."));
#endif
		return;
	}

	AAIController* AIController = Cast<AAIController>(Controller);
	if (AIController == nullptr)
	{
		TFunction<AAIController*(AActor*)> FuncSearchChildren = [&](AActor* Root) -> AAIController*
		{
			for (AActor* Child : Root->Children)
			{
				if (AAIController* ChildController = Cast<AAIController>(Child))
				{
					return ChildController;
				}
				if (AAIController* ChildController = FuncSearchChildren(Child))
				{
					return ChildController;
				}
			}
			return nullptr;
		};
		AIController = FuncSearchChildren(Controller);
	}

	UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("UDeterministicBlackboardValues::ApplyValues Pawn:%s Controller:%s"), *Pawn->GetName(), Controller ? *Controller->GetName() : TEXT("Unset"));

	if (AIController)
	{
		UBlackboardComponent* Blackboard = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		if (Blackboard == nullptr)
		{
			UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("No blackboard, retrying in 1s. Pawn:%s Controller:%s"), *GetNameSafe(Pawn), *GetNameSafe(Controller));
			return;
		}

		FVector WorldLocation = ((AActor*)Controller)->GetActorLocation();

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

		constexpr float Tolerance = 100.0f; // A tolerance to allow the point to snap to the nav mesh surfaces.

		FNavLocation LocA;
		{
			FVector LocalRandomPoint = WorldLocation + BlackboardValues.TargetAValue;
			bool bResult = NavSys->GetRandomPointInNavigableRadius(LocalRandomPoint, Tolerance, LocA);
			checkf(bResult, TEXT("Could not find a point in nav mesh at %s"), *LocalRandomPoint.ToString());
		}

		FNavLocation LocB;
		{
			FVector LocalRandomPoint = WorldLocation + BlackboardValues.TargetBValue;
			bool bResult = NavSys->GetRandomPointInNavigableRadius(LocalRandomPoint, Tolerance, LocB);
			checkf(bResult, TEXT("Could not find a point in nav mesh at %s"), *LocalRandomPoint.ToString());
		}
		
		Blackboard->SetValueAsVector(BlackboardValues.TargetAName, LocA);
		Blackboard->SetValueAsVector(BlackboardValues.TargetBName, LocB);

		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("Setting points to run between as %s and %s for AI controller %s"), *LocA.Location.ToString(), *LocB.Location.ToString(), *Controller->GetName());
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	else
	{
#if !WITH_EDITOR
		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("AI controller not yet available, retrying in 1 second."));
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
