// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkGymPlayerState.h"

bool ABenchmarkGymPlayerState::ShouldBroadCastWelcomeMessage(bool /*bExiting*/)
{
	// We do not want to send notifications between players.
	return false;
}

