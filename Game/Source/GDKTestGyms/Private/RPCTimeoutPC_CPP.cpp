// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutPC_CPP.h"

#include "MaterialArray_CPP.h"
#include "Chaos/PhysicalMaterials.h"
#include "GameFramework/Character.h"

ARPCTimeoutPC_CPP::ARPCTimeoutPC_CPP()
{
	static ConstructorHelpers::FObjectFinder<UMaterial>RedMaterialFinder(TEXT("Material'/Game/SoftReferenceTest/Red.Red'"));
	RedMaterialAsset = RedMaterialFinder.Object;
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
			MyCharacter->GetMesh()->SetMaterial(0,RedMaterialAsset );
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
	UE_LOG(LogTemp,Warning,TEXT("start timer"));
	
	GetWorld()->GetTimerManager().SetTimer(MaterialSetDelay,this, &ARPCTimeoutPC_CPP::SetMaterialAfterDelay, 2.f,false);

}

void ARPCTimeoutPC_CPP::HasValidCharacter()
{
	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());

	if(MyCharacter)
	{
		TArray<AActor*> MaterialArrays;
		UGameplayStatics::GetAllActorsOfClass(this, AMaterialArray_CPP::StaticClass(),MaterialArrays);

		if(MaterialArrays.Num() > 0){
			if(AMaterialArray_CPP* MaterialArray = Cast<AMaterialArray_CPP>(MaterialArrays[0]))
			{
				// Not sure how to load this
				UMaterial* Material1 = MaterialArray->Material1.Get();
				if(Material1)
				{
					FTransform Transform = FTransform::Identity;
					Transform.SetRotation(FRotator(0.f,0.f,180.f).Quaternion());

					//not sure what bDeferredFinish is
					UTextRenderComponent* TRC = Cast<UTextRenderComponent>(
						MyCharacter->AddComponentByClass(UTextRenderComponent::StaticClass(),false,Transform,false));

					if (TRC)
					{
						TRC->SetText(TEXT("ERROR : Material already loaded on client, test is invalid"));
						TRC->SetTextRenderColor(FColor(255,255,255,255));
					}
				}
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

	TArray<AActor*> MaterialArrays;
	UGameplayStatics::GetAllActorsOfClass(this, AMaterialArray_CPP::StaticClass(),MaterialArrays);
	if(MaterialArrays.Num() > 0)
	{
		if(AMaterialArray_CPP* MaterialArray = Cast<AMaterialArray_CPP>(MaterialArrays[0]))
		{
			// UMaterial* PlayerMaterial = MaterialArray->Material1.LoadSynchronous();
			UMaterial* PlayerMaterial = MaterialArray->Material1.LoadSynchronous();
			OnSetMaterial(PlayerMaterial);
		}
	}
	
}


