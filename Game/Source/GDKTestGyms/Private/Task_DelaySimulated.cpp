// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Task_DelaySimulated.h"

#include "Engine/World.h"
#include "GameplayTasksComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

//#include "EngineClasses/SpatialGameInstance.h"

UTask_DelaySimulated::UTask_DelaySimulated(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bSimulatedTask = true;
}

void UTask_DelaySimulated::Activate()
{
	UWorld* World = GetWorld();
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, this, &UTask_DelaySimulated::OnFinish, 1.0f, false);

	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("UTask_DelaySimulated::Activate %s"), *GetPathName()));
}

void UTask_DelaySimulated::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	Super::InitSimulatedTask(InGameplayTasksComponent);

	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("UTask_DelaySimulated::InitSimulatedTask %s"), *GetPathName()));

	if (APawn* Pawn = Cast<APawn>(InGameplayTasksComponent.GetOwner()))
	{
		if (Pawn->IsLocallyControlled())
		{
			UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Error: Simulated task is simulating on the owning client %s"), *GetPathName()));
		}
	}
}

UTask_DelaySimulated* UTask_DelaySimulated::TaskDelaySimulated(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner, const uint8 Priority /*= 192*/)
{
	UTask_DelaySimulated* MyTask = NewTaskUninitialized<UTask_DelaySimulated>();
	if (MyTask && TaskOwner.GetInterface() != nullptr)
	{
		MyTask->InitTask(*TaskOwner, Priority);
	}
	return MyTask;
}

void UTask_DelaySimulated::OnFinish()
{
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("UTask_DelaySimulated::OnFinish %s"), *GetPathName()));

	EndTask();

	OnFinishDelegate.Broadcast();
}
