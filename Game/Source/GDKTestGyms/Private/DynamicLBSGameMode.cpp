// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicLBSGameMode.h"
#include "UnrealNetwork.h"
#include "SpatialNetDriver.h"
#include "DynamicGridBasedLBStrategy.h"

ADynamicLBSGameMode::ADynamicLBSGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
}

void ADynamicLBSGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ADynamicLBSGameMode, DynamicLBSInfo, COND_SimulatedOnly);

}

void ADynamicLBSGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		DynamicLBSInfo = GetWorld()->SpawnActor<ADynamicLBSInfo>();
		OnDynamicLBSInfoCreated.ExecuteIfBound(DynamicLBSInfo);
	}
}

void ADynamicLBSGameMode::OnRep_DynamicLBSInfo()
{
	auto NetDriver = StaticCast<USpatialNetDriver*>(GetNetDriver());
	UE_LOG(LogDynamicLBStrategy, Verbose, TEXT("[DynamicLBSGameMode] VirtualWorker[%d] received DynamicLBSInfo change!!!"), NetDriver->VirtualWorkerTranslator->GetLocalVirtualWorkerId());
}
