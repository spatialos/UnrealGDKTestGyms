// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Actor.h"
#include "TestGrid_CPP.generated.h"

UCLASS()
class GDKTESTGYMS_API ATestGrid_CPP : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestGrid_CPP();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRepSoftMaterial)
	TSoftObjectPtr<UMaterial> SoftMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRepSoftMaterialArray)
	TArray<TSoftObjectPtr<UMaterial>> SoftMaterialArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRepMaterial)
	UMaterial* Material;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRepMaterialArray)
	TArray<UMaterial*> MaterialArray;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	USceneComponent* Scene;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Cube1;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Cube2;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Cube3;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Cube4;

	UPROPERTY(EditAnywhere)
	UTextRenderComponent* TextRender1;

	UPROPERTY(EditAnywhere)
	UTextRenderComponent* TextRender2;
	
	UPROPERTY(EditAnywhere)
	UTextRenderComponent* TextRender3;
	
	UPROPERTY(EditAnywhere)
	UTextRenderComponent* TextRender4;

	// void InitializeCubeTextMesh()



	// =====================

	UFUNCTION()
	void OnRepSoftMaterial();

	UFUNCTION()
	void OnRepSoftMaterialArray();

	UFUNCTION()
	void OnRepMaterial();

	UFUNCTION()
	void OnRepMaterialArray();

	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


};
