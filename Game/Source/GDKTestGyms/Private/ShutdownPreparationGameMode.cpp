// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "ShutdownPreparationGameMode.h"
#include "EngineClasses/SpatialGameInstance.h"

#include "Misc/EngineVersionComparison.h"

DEFINE_LOG_CATEGORY(LogShutdownPreparationGameMode);

AShutdownPreparationGameMode::AShutdownPreparationGameMode()
	: Super()
{
#if UE_VERSION_NEWER_THAN(4, 27, -1)
	static ConstructorHelpers::FClassFinder<AActor> PawnClassFinder(TEXT("/Game/Characters/PlayerCharacter_BP"));
	DefaultPawnClass = PawnClassFinder.Class;
#endif
}

void AShutdownPreparationGameMode::BeginPlay()
{
	Super::BeginPlay();

	USpatialGameInstance* GameInstance = GetGameInstance<USpatialGameInstance>();
	GameInstance->OnPrepareShutdown.AddDynamic(this, &AShutdownPreparationGameMode::HandleOnPrepareShutdown);
}

void AShutdownPreparationGameMode::HandleOnPrepareShutdown()
{
	UWorld* World = GetWorld();
	UE_LOG(LogShutdownPreparationGameMode, Log, TEXT("Num player controllers: %d"), World->GetNumPlayerControllers());

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
	UE_LOG(LogShutdownPreparationGameMode, Log, TEXT("Controllers returned: %d"), NumControllersReturned);
}

void AShutdownPreparationGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty())
	{
		USpatialGameInstance* GameInstance = GetGameInstance<USpatialGameInstance>();
		if (GameInstance->IsPreparingForShutdown())
		{
			ErrorMessage = TEXT("preparing_for_shutdown");
			return;
		}
	}
}