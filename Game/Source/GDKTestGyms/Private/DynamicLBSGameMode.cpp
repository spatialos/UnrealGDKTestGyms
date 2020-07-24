// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicLBSGameMode.h"
#include "UnrealNetwork.h"
#include "SpatialNetDriver.h"

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

void ADynamicLBSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	DynamicLBSInfo = NewObject<ADynamicLBSInfo>(this);
}

void ADynamicLBSGameMode::OnRep_DynamicLBSInfo()
{
	auto NetDriver = StaticCast<USpatialNetDriver*>(GetNetDriver());
	UE_LOG(LogDynamicLBStrategy, Warning, TEXT("[DynamicLBSGameMode] VirtualWorker[%d] received DynamicLBSInfo change!!!"), NetDriver->VirtualWorkerTranslator->GetLocalVirtualWorkerId());
}
