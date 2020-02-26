// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArtificialLoadCharacter.generated.h"

UCLASS()
class GDKTESTGYMS_API AArtificialLoadCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	float ArtificialWorkload{ 0.0f };

	// Sets default values for this character's properties
	AArtificialLoadCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
