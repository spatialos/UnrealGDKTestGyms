// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutPC.h"


#include "Chaos/PhysicalMaterials.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

ARPCTimeoutPC::ARPCTimeoutPC()
{
	//Choose materials which belong to the Engine. This is in anticipation of the possibility of moving this test to the UnrealGDK plugin in the future.

	TCHAR* FailedMaterialPathString = TEXT("Material'/Engine/EngineDebugMaterials/VertexColorViewMode_RedOnly.VertexColorViewMode_RedOnly'");
	static ConstructorHelpers::FObjectFinder<UMaterial>FailedMaterialFinder(FailedMaterialPathString);
	FailedMaterialAsset = FailedMaterialFinder.Object;
	checkf(IsValid(FailedMaterialAsset), TEXT("Could not find failed material asset %ls"), FailedMaterialPathString);
	
	TCHAR* SoftMaterialPathString = TEXT("Material'/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP_Mat.TutorialTPP_Mat'");
	SoftMaterialPath = FSoftObjectPath(SoftMaterialPathString);
	SoftMaterialPtr = TSoftObjectPtr<UMaterial>(SoftMaterialPath);
}

void ARPCTimeoutPC::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CheckMaterialLoaded();

	//Delay set material, let CheckMaterialLoaded() check if it's already loaded in memory
	GetWorld()->GetTimerManager().SetTimer(MaterialSetDelay,this, &ARPCTimeoutPC::SetMaterialAfterDelay, 2.f,false);
}

void ARPCTimeoutPC::CheckMaterialLoaded_Implementation()
{
	GetWorld()->GetTimerManager().SetTimer(HasValidCharacterTimer,this, &ARPCTimeoutPC::CheckValidCharacter, 0.001,false);
}

void ARPCTimeoutPC::SetMaterialAfterDelay()
{
	UMaterial* PlayerMaterial = SoftMaterialPtr.LoadSynchronous();
	OnSetMaterial(PlayerMaterial);
}

void ARPCTimeoutPC::CheckValidCharacter()
{
	if(ACharacter* TestCharacter = Cast<ACharacter>(GetPawn()))
	{
		if(UMaterial* Material = SoftMaterialPtr.Get())
		{
			FTransform Transform = FTransform::Identity;
			Transform.SetRotation(FRotator::ZeroRotator.Quaternion());

			UTextRenderComponent* TRC = Cast<UTextRenderComponent>(
				TestCharacter->AddComponentByClass(UTextRenderComponent::StaticClass(), false, Transform, false));

			if (TRC)
			{
				TRC->SetText(FText::FromString("ERROR : Material already loaded on client, test is invalid"));
				TRC->SetTextRenderColor(FColor::Red);
			}
		}
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(HasValidCharacterTimer,this, &ARPCTimeoutPC::CheckValidCharacter, 0.001,false);
	}
}

void ARPCTimeoutPC::OnSetMaterial_Implementation(UMaterial* PlayerMaterial)
{
	if(ACharacter* TestCharacter = Cast<ACharacter>(GetPawn()))
	{
		if(PlayerMaterial)
		{
			TestCharacter->GetMesh()->SetMaterial(0,PlayerMaterial);
		}
		else
		{
			TestCharacter->GetMesh()->SetMaterial(0,FailedMaterialAsset);
		}
	}
}
