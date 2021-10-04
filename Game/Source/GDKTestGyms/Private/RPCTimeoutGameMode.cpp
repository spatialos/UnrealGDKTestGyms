// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutGameMode.h"

#include "RPCTimeoutCharacter.h"
#include "RPCTimeoutPC_CPP.h"
#include "GameFramework/Character.h"
#include "GameFramework/DefaultPawn.h"
#include "GDKTestGyms/GDKTestGymsCharacter.h"
#include "SpatialGDKFunctionalTests/SpatialGDK/TestActors/TestMovementCharacter.h"

ARPCTimeoutGameMode::ARPCTimeoutGameMode()
	: Super()
{

	
	PlayerControllerClass = ARPCTimeoutPC_CPP::StaticClass();
	DefaultPawnClass = ARPCTimeoutCharacter::StaticClass();
}
