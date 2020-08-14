// Fill out your copyright notice in the Description page of Project Settings.


#include "ShutdownPreparationGameMode.h"
#include "EngineClasses/SpatialGameInstance.h"

void AShutdownPreparationGameMode::BeginPlay()
{
	Super::BeginPlay();

	USpatialGameInstance* GameInstance = GetGameInstance<USpatialGameInstance>();
	GameInstance->OnPrepareShutdown.AddDynamic(this, &AShutdownPreparationGameMode::HandleOnPrepareShutdown);
}

void AShutdownPreparationGameMode::HandleOnPrepareShutdown()
{
	UWorld* World = GetWorld();
	UE_LOG(LogTemp, Log, TEXT("Num player controllers: %d"), World->GetNumPlayerControllers());

	int NumControllersReturned = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		if (Controller && Controller->HasAuthority() && Controller->IsPrimaryPlayer()) // IsPrimaryPlayer check taken from AGameSession::ReturnToMainMenuHost
		{
			Controller->ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("Deployment is preparing for shutdown.")));
			NumControllersReturned++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Controllers returned: %d"), NumControllersReturned);
}