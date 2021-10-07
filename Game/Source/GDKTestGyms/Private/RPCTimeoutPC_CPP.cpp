// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutPC_CPP.h"

#include "Chaos/PhysicalMaterials.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

ARPCTimeoutPC_CPP::ARPCTimeoutPC_CPP()
{
	static ConstructorHelpers::FObjectFinder<UMaterial>FailedMaterialFinder(TEXT("Material'/Engine/EngineDebugMaterials/VertexColorViewMode_RedOnly.VertexColorViewMode_RedOnly'"));
	FailedMaterialAsset = FailedMaterialFinder.Object;
	
	SoftMaterialPath = FSoftObjectPath(TEXT("Material'/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP_Mat.TutorialTPP_Mat'"));
	SoftMaterialPtr = TSoftObjectPtr<UMaterial>(SoftMaterialPath);
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
				TRC->SetText(FText::FromString("ERROR : Material already loaded on client, test is invalid"));
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
	UMaterial* PlayerMaterial = SoftMaterialPtr.LoadSynchronous();
	OnSetMaterial(PlayerMaterial);
}
