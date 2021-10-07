// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutPC.h"


#include "Chaos/PhysicalMaterials.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

ARPCTimeoutPC::ARPCTimeoutPC()
{
	static ConstructorHelpers::FObjectFinder<UMaterial>FailedMaterialFinder(TEXT("Material'/Engine/EngineDebugMaterials/VertexColorViewMode_RedOnly.VertexColorViewMode_RedOnly'"));
	FailedMaterialAsset = FailedMaterialFinder.Object;
	
	SoftMaterialPath = FSoftObjectPath(TEXT("Material'/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP_Mat.TutorialTPP_Mat'"));
	SoftMaterialPtr = TSoftObjectPtr<UMaterial>(SoftMaterialPath);
}

void ARPCTimeoutPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARPCTimeoutPC::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CheckMaterialLoaded();
	GetWorld()->GetTimerManager().SetTimer(MaterialSetDelay,this, &ARPCTimeoutPC::SetMaterialAfterDelay, 2.f,false);
}

void ARPCTimeoutPC::CheckMaterialLoaded_Implementation()
{
	GetWorld()->GetTimerManager().SetTimer(HasValidCharacterTimer,this, &ARPCTimeoutPC::HasValidCharacter, 0.001,false);
}

void ARPCTimeoutPC::SetMaterialAfterDelay()
{
	UMaterial* PlayerMaterial = SoftMaterialPtr.LoadSynchronous();
	OnSetMaterial(PlayerMaterial);
}

void ARPCTimeoutPC::HasValidCharacter()
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
		GetWorld()->GetTimerManager().SetTimer(HasValidCharacterTimer,this, &ARPCTimeoutPC::HasValidCharacter, 0.001,false);
	}
}

void ARPCTimeoutPC::OnSetMaterial_Implementation(UMaterial* PlayerMaterial)
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
