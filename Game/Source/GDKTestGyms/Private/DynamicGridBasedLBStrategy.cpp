// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicGridBasedLBStrategy.h"
#include "SpatialDebugger.h"
#include "Utils/InspectionColors.h"
#include "SpatialNetDriver.h"
#include "SpatialActorUtils.h"
#include "DynamicLBSGameMode.h"

DEFINE_LOG_CATEGORY(LogDynamicLBStrategy);

void UDynamicGridBasedLBStrategy::Init()
{
	Super::Init();

	NetDriver = StaticCast<USpatialNetDriver*>(GetWorld()->GetNetDriver());
	DynamicLBSGameMode = StaticCast<ADynamicLBSGameMode*>(GetWorld()->GetAuthGameMode());
	DynamicLBSGameMode->OnDynamicLBSInfoCreated.BindUObject(this, &UDynamicGridBasedLBStrategy::InitDynamicLBSInfo);
}

void UDynamicGridBasedLBStrategy::InitDynamicLBSInfo(ADynamicLBSInfo* DynamicLBSInfo)
{
	DynamicLBSInfo->Init(WorkerCells);
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

	ADynamicLBSInfo* DynamicLBSInfo = DynamicLBSGameMode->DynamicLBSInfo;
	if (DynamicLBSInfo == nullptr)
		return AuthorityWorkerId;

	const TArray<FBox2D>& DynamicWorkerCells = DynamicLBSInfo->GetWorkerCells();
	if (DynamicWorkerCells.Num() == 0)
		return AuthorityWorkerId;

	AuthorityWorkerId = SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
	for (int i = 0; i < DynamicWorkerCells.Num(); i++)
	{
		if (IsInside(DynamicWorkerCells[i], Actor2DLocation))
		{
			AuthorityWorkerId = VirtualWorkerIds[i];
			break;
		}
	}

	UE_LOG(LogDynamicLBStrategy, Verbose, TEXT("VirtualWorker[%d] called WhoShouldHaveAuthority() of Actor %s: %d"), LocalVirtualWorkerId, *Actor.GetFName().ToString(), AuthorityWorkerId);

	return AuthorityWorkerId;

}

bool UDynamicGridBasedLBStrategy::ShouldHaveAuthority(const AActor& Actor) const
{
	bool ShouldHaveAuthroity = Super::ShouldHaveAuthority(Actor);
	if (!Actor.IsA(ActorClassToMonitor))
	{
		// We only cares actors of the specific type
		return ShouldHaveAuthroity;
	}

	ADynamicLBSInfo* DynamicLBSInfo = DynamicLBSGameMode->DynamicLBSInfo;
	if (DynamicLBSInfo == nullptr)
		return ShouldHaveAuthroity;

	// Store the actor's position for checking if the actor is moving in/out of worker bounds
	const FVector2D* PrevPosPtr = ActorPrevPositions.Find(&Actor);
	const FVector2D PrevPos = PrevPosPtr != nullptr ? *PrevPosPtr : FVector2D::ZeroVector;
	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
	ActorPrevPositions.Add(&Actor, Actor2DLocation);

	if (PrevPosPtr == nullptr)
	{
		// Can always spawn into local worker cell
		if (ShouldHaveAuthroity)
		{
			DynamicLBSInfo->IncreseActorCounter(LocalVirtualWorkerId);
		}
		return ShouldHaveAuthroity;
	}

	TArray<FBox2D>& DynamicWorkerCells = DynamicLBSInfo->GetWorkerCells();
	if (DynamicWorkerCells.Num() == 0)
		return ShouldHaveAuthroity;

	//UE_LOG(LogDynamicLBStrategy, Verbose, TEXT("VirtualWorker[%d] called ShouldHaveAuthority() of Actor %s"), LocalVirtualWorkerId, *Actor.GetFName().ToString());

	auto LocalWorkerCell = DynamicWorkerCells[LocalVirtualWorkerId - 1];
	// Leaving local worker cell
	if (!IsInside(LocalWorkerCell, Actor2DLocation))
	{
		if (IsInside(LocalWorkerCell, PrevPos))
		{
			int32 ToWorkerCellIndex = DynamicWorkerCells.IndexOfByPredicate([Actor2DLocation](const FBox2D& Cell)
			{
				return IsInside(Cell, Actor2DLocation);
			});
			if (ToWorkerCellIndex != INDEX_NONE)
			{
				uint32 ToWorkerActorCounter = DynamicLBSInfo->GetActorCounter(ToWorkerCellIndex + 1);
				if (ToWorkerActorCounter >= MaxActorLoad)
				{
					UpdateWorkerBounds(PrevPos, Actor2DLocation, LocalVirtualWorkerId - 1, ToWorkerCellIndex);
					return true;
				}
			}
		}
		else
		{
			// Local worker cell is changed by the movement of other actor.
			// In this case, the actor will lose authority.
			return false;
		}
	}
	// Entering local worker cell
	else if (!IsInside(LocalWorkerCell, PrevPos))
	{
		uint32 ActorCounter = DynamicLBSInfo->GetActorCounter(LocalVirtualWorkerId);
		if (ActorCounter >= MaxActorLoad)
		{
			int32 ToWorkerCellIndex = LocalVirtualWorkerId - 1;
			int32 FromWorkerCellIndex = DynamicWorkerCells.IndexOfByPredicate([PrevPos](const FBox2D& Cell)
			{
				return IsInside(Cell, PrevPos);
			});

			// Update worker cell bounds (both local worker and the worker where the actor comes from)
			UpdateWorkerBounds(PrevPos, Actor2DLocation, FromWorkerCellIndex, ToWorkerCellIndex);

			return false;
		}
		else
		{
			DynamicLBSInfo->IncreseActorCounter(LocalVirtualWorkerId);
		}
	}

	return true;
}

void UDynamicGridBasedLBStrategy::UpdateWorkerBounds(const FVector2D PrevPos, const FVector2D Actor2DLocation, const int32 FromWorkerCellIndex, const int32 ToWorkerCellIndex) const
{
	if (FromWorkerCellIndex == INDEX_NONE || ToWorkerCellIndex == INDEX_NONE)
		return;

	ADynamicLBSInfo* DynamicLBSInfo = DynamicLBSGameMode->DynamicLBSInfo;
	if (DynamicLBSInfo == nullptr)
		return;

	TArray<FBox2D>& DynamicWorkerCells = DynamicLBSInfo->GetWorkerCells();
	auto FromWorkerCell = DynamicWorkerCells[FromWorkerCellIndex];
	auto ToWorkerCell = DynamicWorkerCells[ToWorkerCellIndex];

	// X-axis dynamic change
	if (PrevPos.X < ToWorkerCell.Min.X || PrevPos.X > ToWorkerCell.Max.X)
	{
		// Coming from left region
		if (ToWorkerCell.GetCenter().X > Actor2DLocation.X)
		{
			ToWorkerCell.Min.X = Actor2DLocation.X + BoundaryChangeStep;
			FromWorkerCell.Max.X = ToWorkerCell.Min.X;
		}
		else
		{
			ToWorkerCell.Max.X = Actor2DLocation.X - BoundaryChangeStep;
			FromWorkerCell.Min.X = ToWorkerCell.Max.X;
		}
	}
	// Y-axis dynamic change
	else if (PrevPos.Y < ToWorkerCell.Min.Y || PrevPos.Y > ToWorkerCell.Max.Y)
	{
		// Coming from bottom region
		if (ToWorkerCell.GetCenter().Y > Actor2DLocation.Y)
		{
			ToWorkerCell.Min.Y = Actor2DLocation.Y + BoundaryChangeStep;
			FromWorkerCell.Max.Y = ToWorkerCell.Min.Y;
		}
		else
		{
			ToWorkerCell.Max.Y = Actor2DLocation.Y - BoundaryChangeStep;
			FromWorkerCell.Min.Y = ToWorkerCell.Max.Y;
		}
	}
	DynamicWorkerCells[FromWorkerCellIndex] = FromWorkerCell;
	DynamicWorkerCells[ToWorkerCellIndex] = ToWorkerCell;
	//DynamicLBSInfo->DynamicWorkerCells = DynamicWorkerCells;
	DynamicLBSInfo->UpdateWorkerCells(DynamicWorkerCells);

	UE_LOG(LogDynamicLBStrategy, Log, TEXT("VirtualWorker[%d] updated worker cells: [%d] = %s, [%d] = %s"), LocalVirtualWorkerId, FromWorkerCellIndex, *FromWorkerCell.ToString(), ToWorkerCellIndex, *ToWorkerCell.ToString());

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
}
