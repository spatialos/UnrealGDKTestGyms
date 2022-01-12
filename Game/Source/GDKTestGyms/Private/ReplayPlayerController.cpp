// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "ReplayPlayerController.h"
#include "Components/InputComponent.h"
#include "Framework/Commands/InputChord.h"

AReplayPlayerController::AReplayPlayerController()
{
}

void AReplayPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AReplayPlayerController::StartRecordingButtonPressed);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AReplayPlayerController::StopRecordingButtonPressed);
	InputComponent->BindKey(EKeys::L, IE_Pressed, this, &AReplayPlayerController::SwitchRecordingModeButtonPressed);
}

void AReplayPlayerController::StartRecordingButtonPressed()
{
	if (bRecordLocally)
	{
		StartRecordingRPC_Implementation();
	}
	else
	{
		ReplayNumber++;
		StartRecordingRPC();
	}
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Started a recording with the filename: %s."), *GetRecordingFilename()));
	}
}

void AReplayPlayerController::StopRecordingButtonPressed()
{
	if (bRecordLocally)
	{
		StopRecordingRPC_Implementation();
	}
	else
	{
		StopRecordingRPC();
	}
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Stopped recording with the filename: %s."), *GetRecordingFilename()));
	}
}



void AReplayPlayerController::SwitchRecordingModeButtonPressed()
{
	bRecordLocally = !bRecordLocally;
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Switched the recording mode, now recording on the %s."), bRecordLocally ? TEXT("Client") : TEXT("Server")));
	}
}

FString AReplayPlayerController::GetRecordingFilename()
{
	return FString::Printf(TEXT("Replay%s%d"), bRecordLocally ? TEXT("Client") : TEXT("Server"), ReplayNumber);
}

void AReplayPlayerController::StartRecordingRPC_Implementation()
{
	ReplayNumber++;
	GetGameInstance()->StartRecordingReplay(GetRecordingFilename(), TEXT(""));
}

void AReplayPlayerController::StopRecordingRPC_Implementation()
{
	GetGameInstance()->StopRecordingReplay();
}

