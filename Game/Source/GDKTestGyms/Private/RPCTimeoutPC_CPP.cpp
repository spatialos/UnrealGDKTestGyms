// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutPC_CPP.h"

#include "MaterialArray_CPP.h"
#include "Chaos/PhysicalMaterials.h"
#include "GameFramework/Character.h"

ARPCTimeoutPC_CPP::ARPCTimeoutPC_CPP()
{
}

void ARPCTimeoutPC_CPP::OnSetMaterial(UMaterial* PlayerMaterial)
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
			// Material class in BP
			MyCharacter->GetMesh()->SetMaterial(0,PlayerMaterial );
		}
	}
}

void ARPCTimeoutPC_CPP::OnPossess(APawn* InPawn)
{
	// Super::OnPossess(InPawn);
	CheckMaterialLoaded();
	
}

void ARPCTimeoutPC_CPP::CheckMaterialLoaded()
{
	GetWorld()->GetTimerManager().SetTimer(MaterialTimerHandle,this, &ARPCTimeoutPC_CPP::HasValidCharacter, 0.001,false,0);
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
				UMaterial* Material1 = MaterialArray->Material1.LoadSynchronous();
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
						TRC->SetTextRenderColor(FColor(0,0,255,255));
					}
				}
			}
		}
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(MaterialTimerHandle,this, &ARPCTimeoutPC_CPP::HasValidCharacter, 0.001,false,0);
	}
}

