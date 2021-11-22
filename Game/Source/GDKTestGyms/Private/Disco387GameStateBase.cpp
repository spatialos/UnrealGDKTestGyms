// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "Disco387GameStateBase.h"

#include "SpatialGDKSettings.h"

DEFINE_LOG_CATEGORY(LogDisco387GameStateBase);

void ADisco387GameStateBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Modify settings here that are desired for this scenario. We modify them here so that it's possible
	// to run a single assembly against multiple scenarios. This might not actually be necessary, in which
	// case we can incorporate these changes into the config file directly.
	if (USpatialGDKSettings* GDKSettings = GetMutableDefault<USpatialGDKSettings>())
	{
		GDKSettings->bEnableNetCullDistanceFrequency = false;
		GDKSettings->bWorkerFlushAfterOutgoingNetworkOp = false;

		GDKSettings->bUseClientEntityInterestQueries = true;
		GDKSettings->bUseNarrowPhaseNCDInterestCulling = true;
		GDKSettings->bUserSpaceServerInterest = true;

		// Loudly output this information to help prevent tripping people up.
		UE_LOG(LogDisco387GameStateBase, Warning, TEXT("Explicit disco config overrides:"));
		UE_LOG(LogDisco387GameStateBase, Warning, TEXT("bEnableNetCullDistanceFrequency is %s."), GDKSettings->bEnableNetCullDistanceFrequency ? TEXT("enabled") : TEXT("disabled"));
		UE_LOG(LogDisco387GameStateBase, Warning, TEXT("bWorkerFlushAfterOutgoingNetworkOp is %s."), GDKSettings->bWorkerFlushAfterOutgoingNetworkOp ? TEXT("enabled") : TEXT("disabled"));
		UE_LOG(LogDisco387GameStateBase, Warning, TEXT("bUseClientEntityInterestQueries is %s."), GDKSettings->bUseClientEntityInterestQueries ? TEXT("enabled") : TEXT("disabled"));
		UE_LOG(LogDisco387GameStateBase, Warning, TEXT("bUseNarrowPhaseNCDInterestCulling is %s."), GDKSettings->bUseNarrowPhaseNCDInterestCulling ? TEXT("enabled") : TEXT("disabled"));
		UE_LOG(LogDisco387GameStateBase, Warning, TEXT("bUserSpaceServerInterest is %s."), GDKSettings->bUserSpaceServerInterest ? TEXT("enabled") : TEXT("disabled"));
	}
}
