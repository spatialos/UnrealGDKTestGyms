// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "BenchmarkNPCCharacter.h"
/*
void ABenchmarkNPCCharacter::OnAuthorityGained()
{
	Super::OnAuthorityGained();
}
*/
void ABenchmarkNPCCharacter::OnAuthorityLost()
{
	Super::OnAuthorityLost();

	if (Controller)
	{
		Controller->Destroy();
	}
}


