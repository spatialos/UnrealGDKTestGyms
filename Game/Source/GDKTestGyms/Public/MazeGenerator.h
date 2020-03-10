// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeGenerator.generated.h"

USTRUCT(BlueprintType)
struct FActorDistribution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	int NumberToSpawn;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	FVector LocalOffset;

	FActorDistribution() = default;
};

UCLASS()
class GDKTESTGYMS_API AMazeGenerator : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	class UInstancedStaticMeshComponent* Walls;

public:	
	// Sets default values for this actor's properties
	AMazeGenerator();

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	float WidthInMetres;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	float LengthInMetres;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	int Rows;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	int Cols;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	FVector DefaultWallScale;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	float CreateWallWeight;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	TArray<FActorDistribution> ActorDistributions;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	bool bSpawnActorDistributions;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	int Seed;

	UPROPERTY(BlueprintReadWrite)
	FRandomStream RandomStream;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void AddWall(float X, float Y, float Length, bool bRotate);

	UFUNCTION(BlueprintCallable)
	void AddInnerWalls();
};
