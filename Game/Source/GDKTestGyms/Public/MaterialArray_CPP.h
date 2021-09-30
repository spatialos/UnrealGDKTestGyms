// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "TestGrid_CPP.h"
#include "GameFramework/Actor.h"
#include "MaterialArray_CPP.generated.h"

UCLASS()
class GDKTESTGYMS_API AMaterialArray_CPP : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMaterialArray_CPP();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	ATestGrid_CPP* TestGrid;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> Material1;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> Material2;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> Material3;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> Material4;
};
