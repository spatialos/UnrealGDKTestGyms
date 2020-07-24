#pragma once

#include "SpatialCommonTypes.h"

#include "GameFramework/Info.h"
#include "Math/Box2D.h"

#include "DynamicLBSInfo.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDynamicLBStrategy, Log, All)

class USpatialNetDriver;

UCLASS(SpatialType = (ServerOnly), Blueprintable, NotPlaceable)
class GDKTESTGYMS_API ADynamicLBSInfo : public AInfo
{
	GENERATED_UCLASS_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void Init(const TArray<FBox2D> WorkerCells);
	virtual void OnAuthorityGained() override;

	TArray<FBox2D>& GetWorkerCells() { return DynamicWorkerCells; }

	uint32 GetActorCounter(const VirtualWorkerId TargetWorkerId);

	UFUNCTION(CrossServer, Reliable)
	void IncreseActorCounter(const uint32 TargetWorkerId);
	UFUNCTION(CrossServer, Reliable)
	void DecreaseActorCounter(const uint32 TargetWorkerId);
	UFUNCTION(CrossServer, Reliable)
	void UpdateWorkerCells(const TArray<FBox2D>& NewWorkerCells);

private:

	UPROPERTY(ReplicatedUsing = OnRep_DynamicWorkerCells)
	TArray<FBox2D> DynamicWorkerCells;

	UPROPERTY(ReplicatedUsing = OnRep_ActorCounters)
	TArray<uint32> ActorCounters;

	UFUNCTION()
	void OnRep_DynamicWorkerCells();
	UFUNCTION()
	void OnRep_ActorCounters();

	USpatialNetDriver* NetDriver;

};
