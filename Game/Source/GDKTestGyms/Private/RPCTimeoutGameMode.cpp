// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutGameMode.h"

#include "RPCTimeoutCharacter.h"
#include "RPCTimeoutPC_CPP.h"

ARPCTimeoutGameMode::ARPCTimeoutGameMode()
	: Super()
{
	PlayerControllerClass = ARPCTimeoutPC_CPP::StaticClass();
	DefaultPawnClass = ARPCTimeoutCharacter::StaticClass();
}