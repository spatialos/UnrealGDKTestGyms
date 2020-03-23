// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeGenerator.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMapGenerator, Log, All);

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

UCLASS(AutoExpandCategories=("Walls", "Distributed Actors"), HideCategories = ("Transform", "Rendering", "Replication", "Collision", "Authority", "Input", "Actor", "LOD", "Cooking"))
class GDKTESTGYMS_API AMazeGenerator : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	class UInstancedStaticMeshComponent* Walls;

public:	
	// Sets default values for this actor's properties
	AMazeGenerator();

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Maze")
	int Seed;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Maze")
	float WidthInMetres;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Maze")
	float LengthInMetres;
		
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Walls")
	int Rows;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Walls")
	int Cols;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Walls")
	FVector DefaultWallScale;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Walls")
	float CreateWallWeight;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Distributed Actors", meta = (EditFixedOrder))
	TArray<FActorDistribution> ActorDistributions;

	UPROPERTY(BlueprintReadWrite)
	FRandomStream RandomStream;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void AddWall(float X, float Y, float Length, bool bRotate);

	UFUNCTION(BlueprintCallable)
	void AddOuterWalls();

	UFUNCTION(BlueprintCallable)
	void AddInnerWalls();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Walls")
	void ClearMazeWalls();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Walls")
	void GenerateMazeWalls();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Distributed Actors")
	void ClearDistributedActors();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Distributed Actors")
	void SpawnDistributedActors();
};
