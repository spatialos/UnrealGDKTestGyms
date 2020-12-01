// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "ClientTravelController.h"

DEFINE_LOG_CATEGORY_STATIC(LogClient, Log, All);

void AClientTravelController::OnCommandTravel()
{
	UE_LOG(LogClient, Log, TEXT("Travelling"));
	FURL TravelURL;
	TravelURL.Host = TEXT("locator.improbable.io");
	TravelURL.AddOption(TEXT("devauth"));
	TravelURL.AddOption(TEXT("deployment=client_travel_gym")); // optional 
	ClientTravel(TravelURL.ToString(), TRAVEL_Absolute, false /*bSeamless*/);
}

