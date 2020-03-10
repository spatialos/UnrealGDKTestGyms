// Fill out your copyright notice in the Description page of Project Settings.


#include "MazeGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "SpatialGDK/Public/Utils/SpatialLatencyPayload.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AMazeGenerator::AMazeGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	WidthInMetres = 150;
	LengthInMetres = 100;
	Rows = 2;
	Cols = 3;
	DefaultWallScale = FVector(1, 1, 2.5);
	CreateWallWeight = 1;
	Seed = 0;

	Walls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Walls"));
	SetRootComponent(Walls);
	Walls->SetMobility(EComponentMobility::Static);
}

// Called when the game starts or when spawned
void AMazeGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMazeGenerator::AddWall(float X, float Y, float Length, bool bRotate)
{
	FTransform Transform;
	Transform.SetLocation(FVector(X * 100.0, Y * 100.0, DefaultWallScale.Z * 50.0));
	Transform.SetRotation(bRotate ?  FRotator(0, 90, 0).Quaternion() : FQuat::Identity);
	Transform.SetScale3D(FVector(Length + 1, DefaultWallScale.Y, DefaultWallScale.Z));

	Walls->AddInstance(Transform);
}

void AMazeGenerator::AddInnerWalls()
{
	float RowLength = LengthInMetres / Rows;
	float ColWidth = WidthInMetres / Cols;

	for (int RowIndex = 0; RowIndex < Rows; RowIndex++)
	{
		for (int ColIndex = 0; ColIndex < Cols; ColIndex++)
		{
			if (ColIndex < Cols - 1)
			{
				if (UKismetMathLibrary::RandomBoolWithWeightFromStream(CreateWallWeight, RandomStream))
				{
					AddWall(
						RowLength * RowIndex + RowLength / 2.0f - LengthInMetres / 2.0f,
						ColWidth * (ColIndex + 1) - WidthInMetres / 2.0f,
						RowLength,
						false
					);
				}
			}

			if (RowIndex < Rows - 1)
			{
				if (UKismetMathLibrary::RandomBoolWithWeightFromStream(CreateWallWeight, RandomStream))
				{
					AddWall(
						RowLength * RowIndex + RowLength - LengthInMetres / 2.0f,
						ColWidth * ColIndex + ColWidth / 2.0f - WidthInMetres / 2.0f,
						ColWidth,
						true
					);
				}
			}
		}
	}
}