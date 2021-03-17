// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomSchemaSnapshotActor.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Net/UnrealNetwork.h"
#include "Utils/SpatialStatics.h"

const Worker_ComponentId PERSISTENCE_COMPONENT_ID = 9950;
const Schema_FieldId DEPLETED_FIELD_ID = 1;

// Sets default values
ACustomSchemaSnapshotActor::ACustomSchemaSnapshotActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bDepleted = false;
}

// Called when the game starts or when spawned
void ACustomSchemaSnapshotActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACustomSchemaSnapshotActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Import Custom Schema before beginplay() and after component interference
void ACustomSchemaSnapshotActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	OnPostInitComponentsSetupBlueprintFromSnapshot();
}

void ACustomSchemaSnapshotActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(ACustomSchemaSnapshotActor, bDepleted);
}

void ACustomSchemaSnapshotActor::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	// Work with spatial turned off
	if (!USpatialStatics::IsSpatialNetworkingEnabled())
	{
		return;
	}
	
	const uint64 EntityID = USpatialStatics::GetActorEntityId(this);
	if (EntityID == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Don't have an entity ID in PreReplicate."));
		return;
	}

	const USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Running with spatial but can't find a spatial net driver."));
		return;
	}

	if(!NetDriver->Connection->GetCoordinator().HasEntity(EntityID))
	{
		UE_LOG(LogTemp, Warning, TEXT("View coordinator doesn't have entity %llu yet."), EntityID);
		return;
	}

	SpatialGDK::ComponentUpdate Update(PERSISTENCE_COMPONENT_ID);
	FillSchemaData(*Update.GetFields());
  NetDriver->Connection->GetCoordinator().SendComponentUpdate(EntityID, Update.DeepCopy(), {});
	
	// Worker_ComponentUpdate Update = {};
	// Update.component_id = PERSISTENCE_COMPONENT_ID;
	// Update.schema_type = Schema_CreateComponentUpdate();
	// Schema_Object* ComponentObject = Schema_GetComponentUpdateFields(Update.schema_type);
	//
	// FillSchemaData(*ComponentObject);
	// FWorkerComponentUpdate SendUpdate(Update);
	// NetDriver->Connection->SendComponentUpdate(EntityID, &SendUpdate);
}

void ACustomSchemaSnapshotActor::FillSchemaData(Schema_Object& ComponentObject) const
{
	Schema_AddBool(&ComponentObject, DEPLETED_FIELD_ID, bDepleted);
}


