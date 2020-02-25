// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTask.h"

#include "Task_DelaySimulated.generated.h"

UCLASS()
class GDKTESTGYMS_API UTask_DelaySimulated : public UGameplayTask
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelaySimulatedDelegate);

public:
	UTask_DelaySimulated(const FObjectInitializer& ObjectInitializer);

	virtual void Activate() override;

	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;

	UFUNCTION(BlueprintCallable, Category = "SimulatedGameplayTask", meta = (BlueprintInternalUseOnly = "TRUE"))
	static UTask_DelaySimulated* TaskDelaySimulated(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner, const uint8 Priority = 192);

	UPROPERTY(BlueprintAssignable)
	FDelaySimulatedDelegate OnFinishDelegate;

private:
	void OnFinish();
};
