#include "BenchmarkGymPlayerState.h"

ABenchmarkGymPlayerState::ABenchmarkGymPlayerState()
{

}

bool ABenchmarkGymPlayerState::ShouldBroadCastWelcomeMessage(bool bExiting)
{
	//we do not want to send notifications between players
	return false;
}

