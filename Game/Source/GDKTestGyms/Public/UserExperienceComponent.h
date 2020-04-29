// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UserExperienceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserExperienceComponent, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UUserExperienceComponent : public UActorComponent
{
	GENERATED_BODY()
	// Sets default values for this component's properties
	UUserExperienceComponent();

	static constexpr int NumWindowSamples = 20;
public:	
	virtual void InitializeComponent()
	{
		ServerTime = 0.0f;
		ClientTimeSinceServerUpdate = LastServerUpdate = 0.0f;
	}
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float ServerTime;
	float Syncronisity;

	float LastServerUpdate;
	float ClientTimeSinceServerUpdate;
	
	TArray<float> ClientUpdateFrequency;
	TArray<float> RoundTripTime;

	UFUNCTION(Client, Reliable)
	void ClientUpdate(float ServerTime);

	UFUNCTION(Server, Reliable)
	void ServerResponse(float ServerTime);

	UFUNCTION(Server, Reliable)
	void ServerLog(const FString& Message);
};
