// Fill out your copyright notice in the Description page of Project Settings.


#include "DeterministicBlackboardValues.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY(LogDeterministicBlackboardValues);

// Sets default values for this component's properties
UDeterministicBlackboardValues::UDeterministicBlackboardValues()
{
	bWantsInitializeComponent = true;
	//PrimaryComponentTick.bWantsInitializeComponent = true;
}

void UDeterministicBlackboardValues::InitializeComponent() 
{
	Super::InitializeComponent();
}

// Called every frame
void UDeterministicBlackboardValues::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UDeterministicBlackboardValues::ApplyValues()
{
	APawn* P = Cast<APawn>(GetOwner());
	AController* Controller = P->GetController();

	static FName TargetA{ TEXT("TargetA") };
	static FName TargetB{ TEXT("TargetB") };

	AAIController* AIController = Cast<AAIController>(Controller);
	if (AIController == nullptr)
	{
		AIController = Cast<AAIController>(P);
	}
	if (AIController)
	{
		UBlackboardComponent* Blackboard = (UBlackboardComponent*)AIController->GetBlackboardComponent();
		checkf(Blackboard, TEXT("AI Controller did not have a blackboard %s"), *Controller->GetPawn()->GetName());

		FVector WorldLocation = ((AActor*)Controller)->GetActorLocation(); // GetWorldLocation?

	// 	FVector RunLocationA = WorldLocation + Locations.A;
	// 	FVector RunLocationB = WorldLocation + Locations.B;

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

		FNavLocation LocA;
		checkf(NavSys->GetRandomPointInNavigableRadius(WorldLocation + RunPointA, 100.0f, LocA), TEXT("Was not able to find point to nav to."));

		FNavLocation LocB;
		checkf(NavSys->GetRandomPointInNavigableRadius(WorldLocation + RunPointB, 100.0f, LocB), TEXT("Was not able to find point to nav to."));
		
		Blackboard->SetValueAsVector(TargetA, LocA.Location);
		Blackboard->SetValueAsVector(TargetB, LocB.Location);

		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("Setting points to run between as %s and %s for AI controller %s"), *RunPointA.ToString(), *RunPointB.ToString(), *Controller->GetName());
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}

	else
	{
//#if !WITH_EDITOR
		UE_LOG(LogDeterministicBlackboardValues, Log, TEXT("Could not find an AI controller."));
//#endif
	}
}
void UDeterministicBlackboardValues::SetValues_Implementation(const FVector& A, const FVector& B)
{
	RunPointA = A;
	RunPointB = B;

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UDeterministicBlackboardValues::ApplyValues, 1.0f, true);
}