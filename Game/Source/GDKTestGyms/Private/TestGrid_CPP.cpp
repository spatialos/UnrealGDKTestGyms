// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "TestGrid_CPP.h"

#include "Net/UnrealNetwork.h"

// Sets default values
ATestGrid_CPP::ATestGrid_CPP()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh>CubeFinder(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	UStaticMesh* CubeAsset = CubeFinder.Object;
	
	static ConstructorHelpers::FObjectFinder<UMaterial>RedMaterialFinder(TEXT("Material'/Game/SoftReferenceTest/Red.Red'"));
	UMaterial* RedMaterialAsset = RedMaterialFinder.Object;
	
	Scene = CreateDefaultSubobject<USceneComponent>("Scene");
	RootComponent = Scene;
	
	Cube1 = CreateDefaultSubobject<UStaticMeshComponent>("Cube1");
	Cube1->SetupAttachment(Scene);
	Cube1->SetStaticMesh(CubeAsset);
	Cube1->SetMaterial(0,RedMaterialAsset);
	Cube1->SetRelativeLocation(FVector(0.f,-150.f,50.f));
	
	Cube2 = CreateDefaultSubobject<UStaticMeshComponent>("Cube2");
	Cube2->SetupAttachment(Scene);
	Cube2->SetStaticMesh(CubeAsset);
	Cube2->SetMaterial(0,RedMaterialAsset);
	Cube2->SetRelativeLocation(FVector(0.f,-150.f,250.f));
	
	Cube3 = CreateDefaultSubobject<UStaticMeshComponent>("Cube3");
	Cube3->SetupAttachment(Scene);
	Cube3->SetStaticMesh(CubeAsset);
	Cube3->SetMaterial(0,RedMaterialAsset);
	Cube3->SetRelativeLocation(FVector(0.f,150.f,250.f));
	
	Cube4 = CreateDefaultSubobject<UStaticMeshComponent>("Cube4");
	Cube4->SetupAttachment(Scene);
	Cube4->SetStaticMesh(CubeAsset);
	Cube4->SetMaterial(0,RedMaterialAsset);
	Cube4->SetRelativeLocation(FVector(0.f,150.f,50.f));
	
	TextRender1 = CreateDefaultSubobject<UTextRenderComponent>("TextRender1");
	TextRender1->SetupAttachment(Cube1);
	TextRender1->SetText("No rep on material ref");
	TextRender1->SetRelativeLocation(FVector(60.f,0.f,0.f));
	TextRender1->SetHorizontalAlignment(EHTA_Center);
	TextRender1->SetTextRenderColor(FColor::Yellow);
	
	TextRender2 = CreateDefaultSubobject<UTextRenderComponent>("TextRender2");
	TextRender2->SetupAttachment(Cube2);
	TextRender2->SetText("No rep on material ref array");
	TextRender2->SetRelativeLocation(FVector(60.f,0.f,0.f));
	TextRender2->SetHorizontalAlignment(EHTA_Center);
	TextRender2->SetTextRenderColor(FColor::Yellow);
	
	TextRender3 = CreateDefaultSubobject<UTextRenderComponent>("TextRender3");
	TextRender3->SetupAttachment(Cube3);
	TextRender3->SetText("No rep on soft material ref");
	TextRender3->SetRelativeLocation(FVector(60.f,0.f,0.f));
	TextRender3->SetHorizontalAlignment(EHTA_Center);
	TextRender3->SetTextRenderColor(FColor::Yellow);
	
	TextRender4 = CreateDefaultSubobject<UTextRenderComponent>("TextRender4");
	TextRender4->SetupAttachment(Cube4);
	TextRender4->SetText("No rep on soft material ref array");
	TextRender4->SetRelativeLocation(FVector(60.f,0.f,0.f));
	TextRender4->SetHorizontalAlignment(EHTA_Center);
	TextRender4->SetTextRenderColor(FColor::Yellow);
	

	bReplicates = true;
}

// Called when the game starts or when spawned
void ATestGrid_CPP::BeginPlay()
{
	Super::BeginPlay();

}

void ATestGrid_CPP::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME( ATestGrid_CPP, MaterialArray );
}

void ATestGrid_CPP::OnRepSoftMaterial()
{
	if(!HasAuthority())
	{
		UMaterial* LoadedMaterial = SoftMaterial.LoadSynchronous();

		if(LoadedMaterial)
		{
			Cube3->SetMaterial(0,LoadedMaterial);
			TextRender3->SetText(TEXT(""));
		}
		else
		{
			TextRender3->SetText(TEXT("Serializing Soft reference failed"));
		}
	}
}

void ATestGrid_CPP::OnRepSoftMaterialArray()
{
	if(!HasAuthority())
	{
		if(SoftMaterialArray.Num()>0)
		{
			UMaterial* LoadedMaterial = SoftMaterialArray[0].LoadSynchronous();

			if(LoadedMaterial)
			{
				Cube4->SetMaterial(0,LoadedMaterial);
				TextRender4->SetText(TEXT(""));
			}
			else
			{
				TextRender4->SetText(TEXT("Serializing soft reference array failed"));
			}
		}
	}
}

void ATestGrid_CPP::OnRepMaterial()
{
	if(!HasAuthority())
	{
		UMaterial* LoadedMaterial = Material;

		if(LoadedMaterial)
		{
			Cube1->SetMaterial(0,LoadedMaterial);
			TextRender1->SetText(TEXT(""));
		}
		else
		{
			TextRender1->SetText(TEXT("Serializing not loaded material reference failed"));
		}
	}
}

void ATestGrid_CPP::OnRepMaterialArray()
{
	if(!HasAuthority())
	{
		if(MaterialArray.Num()>0)
		{
			UMaterial* LoadedMaterial = MaterialArray[0];

			if(LoadedMaterial)
			{
				Cube2->SetMaterial(0,LoadedMaterial);
				TextRender2->SetText(TEXT(""));
			}
			else
			{
				TextRender2->SetText(TEXT("Serializing not loaded material reference array failed"));
			}
		}
	}
}

// Called every frame
void ATestGrid_CPP::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

