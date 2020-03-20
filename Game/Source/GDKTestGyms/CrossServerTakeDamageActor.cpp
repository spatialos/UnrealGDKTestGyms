// Fill out your copyright notice in the Description page of Project Settings.


#include "CrossServerTakeDamageActor.h"

#include "Engine/EngineTypes.h"

void ACrossServerTakeDamageActor::TestTakeDamage(AController* Controller)
{
	FPointDamageEvent DamageEvent;
	DamageEvent.HitInfo.ImpactPoint = DamagePoint;
	TakeDamage(10.0f, DamageEvent, Controller, this);
}