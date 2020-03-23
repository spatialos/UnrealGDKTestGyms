// Fill out your copyright notice in the Description page of Project Settings.


#include "MazeGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "SpatialGDK/Public/Utils/SpatialLatencyPayload.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetArrayLibrary.h"

DEFINE_LOG_CATEGORY(LogMapGenerator);

// Sets default values
AMazeGenerator::AMazeGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Seed = 0;
	WidthInMetres = 150;
	LengthInMetres = 100;
	Rows = 2;
	Cols = 3;
	DefaultWallScale = FVector(1, 1, 2.5);
	CreateWallWeight = 1;

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

void AMazeGenerator::AddOuterWalls()
{
	AddWall(0, WidthInMetres / 2, LengthInMetres, false);
	AddWall(0, WidthInMetres / -2, LengthInMetres, false);
	AddWall(LengthInMetres / 2, 0, WidthInMetres, true);
	AddWall(LengthInMetres / -2, 0, WidthInMetres, true);
}

void AMazeGenerator::ClearMazeWalls()
{
	Walls->ClearInstances();
}

void AMazeGenerator::GenerateMazeWalls()
{
	RandomStream.Initialize(Seed);
	ClearMazeWalls();
	AddOuterWalls();
	AddInnerWalls();
}

void AMazeGenerator::ClearDistributedActors()
{
	const TArray<USceneComponent*> AttachedComponents = Walls->GetAttachChildren();
	
	for (const auto& Comp : AttachedComponents)
	{
		if (AActor* AttachedOwner = Comp->GetOwner())
		{
			AttachedOwner->Destroy();
		}
	}
}

void AMazeGenerator::SpawnDistributedActors()
{
	ClearDistributedActors();

	RandomStream.Initialize(Seed);

	int MaxSpawnPoints = Rows * Cols;
	TArray<int> SpawnPoints;
	SpawnPoints.Init(0, MaxSpawnPoints);
	for (int i = 0; i < MaxSpawnPoints; i++) {
		SpawnPoints[i] = i;
	}
	for (int i = 0; i < MaxSpawnPoints; i++)
	{
		int index = RandomStream.RandRange(0, MaxSpawnPoints);
		if (i != index)
		{
			SpawnPoints.SwapMemory(i, index);
		}
	}

	int SpawnedCount = 0;

	for (const FActorDistribution& Distribution : ActorDistributions)
	{
		for (int i = 0; i < Distribution.NumberToSpawn; i++)
		{
			if (i + SpawnedCount >= MaxSpawnPoints)
			{
				UE_LOG(LogMapGenerator, Warning, TEXT("Ran out of spawn points trying to spawn %s. Max Spawn Points: %d"), *GetNameSafe(Distribution.ActorClass), MaxSpawnPoints);
				return;
			}

			int X = SpawnPoints[SpawnedCount + i] / Rows;
			int Y = SpawnPoints[SpawnedCount + i] % Rows;
			
			float CellLength = LengthInMetres * 100 / Rows;
			float CellWidth = LengthInMetres * 100 / Cols;
			
			FVector CellOffset = FVector(CellLength * X, CellWidth * Y, 0);
			FVector CellMiddle = FVector(CellLength / 2, CellWidth / 2, 0);
			FVector Corner = GetActorLocation() - FVector(LengthInMetres * 100/2, WidthInMetres * 100/2, 0);

			FVector SpawnLocation = Corner + CellOffset + CellMiddle + Distribution.LocalOffset;

			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(Distribution.ActorClass, SpawnLocation, FRotator::ZeroRotator);
			SpawnedActor->AttachToActor(this, FAttachmentTransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false));
		}
		
		SpawnedCount += Distribution.NumberToSpawn;
	}
}