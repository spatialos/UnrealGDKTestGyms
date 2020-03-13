// Fill out your copyright notice in the Description page of Project Settings.


#include "CrossServerTakeDamageActor.h"

#include "Engine/EngineTypes.h"

void ACrossServerTakeDamageActor::SendDamageRPC(AController* Controller)
{
	FPointDamageEvent DamageEvent;
	DamageEvent.HitInfo.ImpactPoint = DamagePoint;
	TakePointDamageCrossServer(10.0f, DamageEvent, Controller, this);
}