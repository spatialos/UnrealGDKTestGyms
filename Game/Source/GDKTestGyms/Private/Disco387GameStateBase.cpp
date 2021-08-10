// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "Disco387GameStateBase.h"

#include "SpatialGDKSettings.h"

void ADisco387GameStateBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Modify settings here that are desired for this scenario. We modify them here so that it's possible
	// to run a single assembly against multiple scenarios. This might not actually be necessary, in which
	// case we can incorporate these changes into the config file directly.
	if (USpatialGDKSettings* GDKSettings = GetMutableDefault<USpatialGDKSettings>())
	{
		GDKSettings->bEnableNetCullDistanceFrequency = false;

		// Loudly output this information to help prevent tripping people up.
		UE_LOG(LogTemp, Warning, TEXT("Explicit disco config overrides:"));
		UE_LOG(LogTemp, Warning, TEXT("bEnableNetCullDistanceFrequency is %s."), GDKSettings->bEnableNetCullDistanceFrequency ? TEXT("enabled") : TEXT("disabled"));
	}
}
