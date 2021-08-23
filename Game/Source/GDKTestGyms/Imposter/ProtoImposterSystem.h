#pragma once

#include "EngineClasses/SpatialImposterSystem.h"

#include "LoadBalancing/LBDataStorage.h"
#include "LoadBalancing/LegacyLoadbalancingComponents.h"
#include "Schema/UnrealMetadata.h"

#include "ProtoImposterSystem.generated.h"

class AScavengersHubIsmActor;

UCLASS(Blueprintable)
class UProtoImposterSystem : public USpatialImposterSystem
{
	GENERATED_BODY()
public:
	UProtoImposterSystem();

protected:
	virtual TArray<SpatialGDK::FLBDataStorage*> GetData() override;

	virtual void Initialize(FSubsystemCollectionBase&) override;
	virtual void Deinitialize() override;

	void Tick();

	void OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld);

	FDelegateHandle WorldChangedDelegate;
	FDelegateHandle PostTickDispatchDelegate;

	SpatialGDK::FSpatialPositionStorage Positions;
	SpatialGDK::TLBDataStorage<SpatialGDK::UnrealMetadata> ActorMetadata;
	TSet<Worker_EntityId_Key> HighResActors;

	UPROPERTY(EditAnywhere)
	UStaticMesh* ImposterMesh = nullptr;

	UPROPERTY()
	AScavengersHubIsmActor* WorldActor = nullptr;
};
