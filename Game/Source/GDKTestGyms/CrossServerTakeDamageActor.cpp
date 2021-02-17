// Copyright (c) Improbable Worlds Ltd, All Rights Reserved


#include "CrossServerTakeDamageActor.h"

#include "Engine/EngineTypes.h"

void ACrossServerTakeDamageActor::TestTakeDamage(AController* Controller)
{
	FRadialDamageEvent RadialDamageEvent;
	RadialDamageEvent.Params.InnerRadius = 10;
	RadialDamageEvent.Params.OuterRadius = 50;
	RadialDamageEvent.Params.BaseDamage = 10;
	RadialDamageEvent.Params.DamageFalloff = 1;
	RadialDamageEvent.Params.MinimumDamage = 5;
	RadialDamageEvent.Origin = RadialDamageOrigin;
	FHitResult HitResult;
	HitResult.ImpactPoint = { 0, 0, 0 };
	RadialDamageEvent.ComponentHits.Add(HitResult);
	TakeDamage(10.0f, RadialDamageEvent, Controller, this);

	FPointDamageEvent PointDamageEvent;
	PointDamageEvent.HitInfo.ImpactPoint = DamagePoint;
	TakeDamage(10.0f, PointDamageEvent, Controller, this);
}