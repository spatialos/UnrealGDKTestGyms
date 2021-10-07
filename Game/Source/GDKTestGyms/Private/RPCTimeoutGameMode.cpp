// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "RPCTimeoutGameMode.h"

#include "RPCTimeoutCharacter.h"
#include "RPCTimeoutPC.h"

ARPCTimeoutGameMode::ARPCTimeoutGameMode()
	: Super()
{
	PlayerControllerClass = ARPCTimeoutPC::StaticClass();
	DefaultPawnClass = ARPCTimeoutCharacter::StaticClass();
}