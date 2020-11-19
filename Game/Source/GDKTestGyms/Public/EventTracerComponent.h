// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EventTracerComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UEventTracerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEventTracerComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseEventTracing = true;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY(ReplicatedUsing = OnRepTestInt)
	int32 TestInt;

	FTimerHandle TimerHandle;

	UFUNCTION()
	void TimerFunction();

	UFUNCTION(Reliable, Client)
	void RunOnClient();

	UFUNCTION()
	void OnRepTestInt();

	bool OwnerHasAuthority() const;
	
};
