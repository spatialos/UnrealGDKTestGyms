// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutPC_CPP.h"

#include "PackageTools.h"
#include "Chaos/PhysicalMaterials.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

ARPCTimeoutPC_CPP::ARPCTimeoutPC_CPP()
{
	static ConstructorHelpers::FObjectFinder<UMaterial>FailedMaterialFinder(TEXT("Material'/Engine/EngineDebugMaterials/VertexColorViewMode_RedOnly.VertexColorViewMode_RedOnly'"));
	FailedMaterialAsset = FailedMaterialFinder.Object;
	// FailedMaterialAsset->bUsedWithSkeletalMesh = true;
	
	SoftMaterialPath = FSoftObjectPath(TEXT("Material'/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP_Mat.TutorialTPP_Mat'"));
	// SoftMaterialPath = FSoftObjectPath(TEXT("Material'/Engine/EngineDebugMaterials/VertexColorViewMode_GreenOnly.VertexColorViewMode_GreenOnly'"));
	
}

void ARPCTimeoutPC_CPP::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void ARPCTimeoutPC_CPP::OnSetMaterial_Implementation(UMaterial* PlayerMaterial)
{
	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());

	if(MyCharacter)
	{
		if(PlayerMaterial)
		{
			
			MyCharacter->GetMesh()->SetMaterial(0,PlayerMaterial);
		}
		else
		{
			MyCharacter->GetMesh()->SetMaterial(0,FailedMaterialAsset);
		}
	}
}

void ARPCTimeoutPC_CPP::CheckMaterialLoaded_Implementation()
{
	GetWorld()->GetTimerManager().SetTimer(HasValidCharacterTimer,this, &ARPCTimeoutPC_CPP::HasValidCharacter, 0.001,false);

}

void ARPCTimeoutPC_CPP::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	SoftMaterialPtr = TSoftObjectPtr<UMaterial>(SoftMaterialPath);
	CheckMaterialLoaded();
	GetWorld()->GetTimerManager().SetTimer(MaterialSetDelay,this, &ARPCTimeoutPC_CPP::SetMaterialAfterDelay, 2.f,false);
}

void ARPCTimeoutPC_CPP::HasValidCharacter()
{
	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
	
	if(MyCharacter)
	{
		UMaterial* Material1 = SoftMaterialPtr.Get();
		if(Material1)
		{
			FTransform Transform = FTransform::Identity;
			Transform.SetRotation(FRotator(0.f,0.f,0.f).Quaternion());

			UTextRenderComponent* TRC = Cast<UTextRenderComponent>(
				MyCharacter->AddComponentByClass(UTextRenderComponent::StaticClass(),false,Transform,false));

			if (TRC)
			{
				TRC->SetText(TEXT("ERROR : Material already loaded on client, test is invalid"));
				TRC->SetTextRenderColor(FColor(255,0,0,255));
			}
		}
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(HasValidCharacterTimer,this, &ARPCTimeoutPC_CPP::HasValidCharacter, 0.001,false);
	}
}

void ARPCTimeoutPC_CPP::SetMaterialAfterDelay()
{
	UMaterial* PlayerMaterial = StreamableManager.LoadSynchronous<UMaterial>(SoftMaterialPath);
	OnSetMaterial(PlayerMaterial);
}

void ARPCTimeoutPC_CPP::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
	if (MyCharacter) MyCharacter->GetMesh()->SetMaterial(0,FailedMaterialAsset);
	
	// SoftMaterialPtr.Reset();
	StreamableManager.Unload(SoftMaterialPath);

	if(GIsEditor)
	{
		TArray<UPackage*> MaterialPackages;
		MaterialPackages.Add(SoftMaterialPtr.Get()->GetPackage());
		bool bSuccess = UPackageTools::UnloadPackages(MaterialPackages);
		UE_LOG(LogTemp,Warning,TEXT("%d"),bSuccess);
	}
	
	Super::EndPlay(EndPlayReason);
	
}
