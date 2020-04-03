// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeterministicBlackboardValues.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDeterministicBlackboardValues, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UDeterministicBlackboardValues : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDeterministicBlackboardValues();

protected:
	FTimerHandle TimerHandle;
	FVector RunPointA; // Points to run betweeen
	FVector RunPointB;
public:	

	void InitializeComponent() override;

	// Called every frame
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyValues();

	UFUNCTION(Client, Reliable)
	void SetValues(const FVector& A, const FVector& B);
};
