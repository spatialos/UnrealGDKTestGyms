// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicGridBasedLBStrategy.h"
#include "SpatialDebugger.h"
#include "Utils/InspectionColors.h"
#include "SpatialNetDriver.h"
#include "SpatialActorUtils.h"

void UDynamicGridBasedLBStrategy::Init()
{
	Super::Init();

	ActorCounter = 0;
}

bool UDynamicGridBasedLBStrategy::ShouldHaveAuthority(const AActor& Actor)
{
	bool ShouldHaveAuthroity = Super::ShouldHaveAuthority(Actor);
	if (!ShouldHaveAuthroity)
	{
		if (Actor.HasAuthority())
		{
			// Lose authority
			if (Actor.IsA(ActorClassToMonitor))
			{
				DecreaseActorCounter();
			}
		}
		return false;
	}

	if (!Actor.HasAuthority() && /**/Actor.IsA(ActorClassToMonitor))
	{
		if (ActorCounter >= MaxActorLoad)
		{
			const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
			// Update WorkerCells
			// FIXME: only support X-axis dynamic scale
			auto WorkerCell = WorkerCells[LocalVirtualWorkerId - 1];
			// Coming from left region
			if (WorkerCell.GetCenter().X > Actor2DLocation.X)
			{
				WorkerCell.Min.X = Actor2DLocation.X;
			}
			else
			{
				WorkerCell.Max.X = Actor2DLocation.X;
			}

			// Update SpatialDebugger's WorkerRegions
			const USpatialNetDriver* NetDriver = StaticCast<USpatialNetDriver*>(GetWorld()->GetNetDriver());
			auto WorkerRegions = NetDriver->SpatialDebugger->WorkerRegions;
			const LBStrategyRegions LBStrategyRegions = GetLBStrategyRegions();
			WorkerRegions.SetNum(LBStrategyRegions.Num());
			for (int i = 0; i < LBStrategyRegions.Num(); i++)
			{
				const TPair<VirtualWorkerId, FBox2D>& LBStrategyRegion = LBStrategyRegions[i];
				const PhysicalWorkerName* WorkerName = NetDriver->VirtualWorkerTranslator->GetPhysicalWorkerForVirtualWorker(LBStrategyRegion.Key);
				FWorkerRegionInfo WorkerRegionInfo;
				WorkerRegionInfo.Color = (WorkerName == nullptr) ? FColor::Magenta : FColor::MakeRandomColor();//SpatialGDK::GetColorForWorkerName(*WorkerName);
				WorkerRegionInfo.Extents = LBStrategyRegion.Value;
				WorkerRegions[i] = WorkerRegionInfo;
			}

			return false;
		}
		else
		{
			//ActorCounter++;
			IncreseActorCounter();
		}
	}

	return true;
}

void UDynamicGridBasedLBStrategy::IncreseActorCounter()
{
	ActorCounter++;
	UE_LOG(LogTemp, Warning, TEXT("VirtualWorker %d increased actor counter to: %d"), LocalVirtualWorkerId, ActorCounter);
}

void UDynamicGridBasedLBStrategy::DecreaseActorCounter()
{
	if (ActorCounter > 0)
	{
		ActorCounter--;
		UE_LOG(LogTemp, Warning, TEXT("VirtualWorker %d decreased actor counter to: %d"), LocalVirtualWorkerId, ActorCounter);
	}
}
