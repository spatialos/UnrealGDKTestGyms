// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicGridBasedLBStrategy.h"
#include "SpatialDebugger.h"
#include "Utils/InspectionColors.h"
#include "SpatialNetDriver.h"
#include "SpatialActorUtils.h"

DEFINE_LOG_CATEGORY(LogDynamicGridBasedLBStrategy);

void UDynamicGridBasedLBStrategy::Init()
{
	Super::Init();

	ActorCounter = 0;
}

bool UDynamicGridBasedLBStrategy::ShouldHaveAuthority(const AActor& Actor)
{
	bool ShouldHaveAuthroity = Super::ShouldHaveAuthority(Actor);
	if (!Actor.IsA(ActorClassToMonitor))
	{
		// We only cares actors of the specific type
		return ShouldHaveAuthroity;
	}

	const FVector2D* PrevPosPtr = ActorPrevPositions.Find(&Actor);
	const FVector2D PrevPos = PrevPosPtr != nullptr ? *PrevPosPtr : FVector2D::ZeroVector;
	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
	ActorPrevPositions.Add(&Actor, Actor2DLocation);

	if (!ShouldHaveAuthroity)
	{
		if (Actor.HasAuthority())
		{
			// Lose authority
			DecreaseActorCounter();
		}
		return false;
	}
	
	// Spawn into local worker cell
	if (PrevPosPtr == nullptr)
	{
		IncreseActorCounter();
		return true;
	}

	/* This line doesn't worker as the actor always have authority when it comes here...
	if (!Actor.HasAuthority())
	*/

	const USpatialNetDriver* NetDriver = StaticCast<USpatialNetDriver*>(GetWorld()->GetNetDriver());
	auto DynamicWorkerCells = NetDriver->DynamicLBSInfo->DynamicWorkerCells;
	if (DynamicWorkerCells.Num() == 0)
		return ShouldHaveAuthroity;

	// Entering local worker cell
	if (!IsInside(DynamicWorkerCells[LocalVirtualWorkerId - 1], PrevPos))
	{
		if (ActorCounter >= MaxActorLoad)
		{
			// Update worker cell boundaries (both local worker and the worker where the actor comes from)
			auto DynamicWorkerCell = DynamicWorkerCells[LocalVirtualWorkerId - 1];
			auto PrevWorkCellIndex = DynamicWorkerCells.IndexOfByPredicate([PrevPos](const FBox2D& Cell)
			{
				return IsInside(Cell, PrevPos);
			});

			// X-axis dynamic change
			if (PrevPos.X < DynamicWorkerCell.Min.X || PrevPos.X > DynamicWorkerCell.Max.X)
			{
				// Coming from left region
				if (DynamicWorkerCell.GetCenter().X > Actor2DLocation.X)
				{
					DynamicWorkerCell.Min.X = Actor2DLocation.X + BoundaryChangeStep;
					if (PrevWorkCellIndex != INDEX_NONE)
					{
						auto PrevWorkerCell = DynamicWorkerCells[PrevWorkCellIndex];
						PrevWorkerCell.Max.X = DynamicWorkerCell.Min.X;
						DynamicWorkerCells[PrevWorkCellIndex] = PrevWorkerCell;
					}
				}
				else
				{
					DynamicWorkerCell.Max.X = Actor2DLocation.X - BoundaryChangeStep;
					if (PrevWorkCellIndex != INDEX_NONE)
					{
						auto PrevWorkerCell = DynamicWorkerCells[PrevWorkCellIndex];
						PrevWorkerCell.Min.X = DynamicWorkerCell.Max.X;
						DynamicWorkerCells[PrevWorkCellIndex] = PrevWorkerCell;
					}
				}
			}
			// Y-axis dynamic change
			else if (PrevPos.Y < DynamicWorkerCell.Min.Y || PrevPos.Y > DynamicWorkerCell.Max.Y)
			{
				// Coming from bottom region
				if (DynamicWorkerCell.GetCenter().Y > Actor2DLocation.Y)
				{
					DynamicWorkerCell.Min.Y = Actor2DLocation.Y + BoundaryChangeStep;
					if (PrevWorkCellIndex != INDEX_NONE)
					{
						auto PrevWorkerCell = DynamicWorkerCells[PrevWorkCellIndex];
						PrevWorkerCell.Max.Y = DynamicWorkerCell.Min.Y;
						DynamicWorkerCells[PrevWorkCellIndex] = PrevWorkerCell;
					}
				}
				else
				{
					DynamicWorkerCell.Max.Y = Actor2DLocation.Y - BoundaryChangeStep;
					if (PrevWorkCellIndex != INDEX_NONE)
					{
						auto PrevWorkerCell = DynamicWorkerCells[PrevWorkCellIndex];
						PrevWorkerCell.Min.Y = DynamicWorkerCell.Max.Y;
						DynamicWorkerCells[PrevWorkCellIndex] = PrevWorkerCell;
					}
				}
			}
			DynamicWorkerCells[LocalVirtualWorkerId - 1] = DynamicWorkerCell;
			NetDriver->DynamicLBSInfo->DynamicWorkerCells = DynamicWorkerCells;

			UE_LOG(LogDynamicGridBasedLBStrategy, Log, TEXT("Actor entering DynamicWorkerCells[%d] from DynamicWorkerCells[%d], new region: %s"), LocalVirtualWorkerId - 1, PrevWorkCellIndex, *DynamicWorkerCell.ToString());

			// Update SpatialDebugger's WorkerRegions
			auto WorkerRegions = NetDriver->SpatialDebugger->WorkerRegions;
			WorkerRegions.SetNum(DynamicWorkerCells.Num());
			for (int i = 0; i < DynamicWorkerCells.Num(); i++)
			{
				const PhysicalWorkerName* WorkerName = NetDriver->VirtualWorkerTranslator->GetPhysicalWorkerForVirtualWorker(i + 1);
				FWorkerRegionInfo WorkerRegionInfo;
				WorkerRegionInfo.Color = (WorkerName == nullptr) ? FColor::Magenta : WorkerRegions[i].Color;//SpatialGDK::GetColorForWorkerName(*WorkerName);
				WorkerRegionInfo.Extents = DynamicWorkerCells[i];
				WorkerRegions[i] = WorkerRegionInfo;
			}
			NetDriver->SpatialDebugger->WorkerRegions = WorkerRegions;

			return false;
		}
		else
		{
			IncreseActorCounter();
		}
	}

	return true;
}

VirtualWorkerId UDynamicGridBasedLBStrategy::WhoShouldHaveAuthority(const AActor& Actor) const
{
	VirtualWorkerId AuthorityWorkerId = Super::WhoShouldHaveAuthority(Actor);
	// For the specific actors, use the dynamic worker cells for authority check
	// For other actors, use the static worker cells from the base class.
	if (!Actor.IsA(ActorClassToMonitor))
	{
		return AuthorityWorkerId;
	}

	const USpatialNetDriver* NetDriver = StaticCast<USpatialNetDriver*>(GetWorld()->GetNetDriver());
	auto DynamicWorkerCells = NetDriver->DynamicLBSInfo->DynamicWorkerCells;
	if (DynamicWorkerCells.Num() == 0)
		return AuthorityWorkerId;

	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));

	for (int i = 0; i < DynamicWorkerCells.Num(); i++)
	{
		if (IsInside(DynamicWorkerCells[i], Actor2DLocation))
		{
			return VirtualWorkerIds[i];
		}
	}

	return SpatialConstants::INVALID_VIRTUAL_WORKER_ID;

}

void UDynamicGridBasedLBStrategy::IncreseActorCounter()
{
	ActorCounter++;
	UE_LOG(LogDynamicGridBasedLBStrategy, Log, TEXT("VirtualWorker %d increased actor counter to: %d"), LocalVirtualWorkerId, ActorCounter);
}

void UDynamicGridBasedLBStrategy::DecreaseActorCounter()
{
	if (ActorCounter > 0)
	{
		ActorCounter--;
		UE_LOG(LogDynamicGridBasedLBStrategy, Log, TEXT("VirtualWorker %d decreased actor counter to: %d"), LocalVirtualWorkerId, ActorCounter);
	}
}
