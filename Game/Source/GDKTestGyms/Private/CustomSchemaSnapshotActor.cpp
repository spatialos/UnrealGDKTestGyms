// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomSchemaSnapshotActor.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialGameInstance.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Net/UnrealNetwork.h"
#include "SpatialView/EntityView.h"
#include "SpatialView/ViewCoordinator.h"
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
	
	USpatialGameInstance* GameInstance = Cast<USpatialGameInstance>(GetGameInstance());
	if (GameInstance != nullptr)
	{
		GameInstance->OnPersistenceDataAvailable.AddDynamic(this, &ACustomSchemaSnapshotActor::OnPersistenceDataAvailable);
	}
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

	// Will have to see if this ComponentUpdate type makes sense to be user-facing.
	SpatialGDK::ComponentUpdate Update(PERSISTENCE_COMPONENT_ID);
	FillSchemaData(*Update.GetFields());
  NetDriver->Connection->GetCoordinator().SendComponentUpdate(EntityID, Update.DeepCopy(), {}); // This deep copy is probably not the right way to do this. Works for now though.
}

void ACustomSchemaSnapshotActor::OnPersistenceDataAvailable()
{
	if (!HasAuthority())
	{
		return;
	}

	const uint64 EntityID = USpatialStatics::GetActorEntityId(this);
	if (EntityID == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Don't have an entity ID in persistence callback."));
		return;
	}

	const USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(GetWorld()->GetNetDriver());
	if (NetDriver == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Got persistence data callback but can't find a spatial net driver."));
		return;
	}

	SpatialGDK::ViewCoordinator& Coordinator = NetDriver->Connection->GetCoordinator();

	if(!Coordinator.HasEntity(EntityID))
	{
		UE_LOG(LogTemp, Warning, TEXT("View coordinator doesn't have entity %llu during persistence callback."), EntityID);
		return;
	}

	const SpatialGDK::EntityView& View = Coordinator.GetView();
	const SpatialGDK::EntityViewElement* ViewData = View.Find(EntityID);
	if (ViewData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Found no persistence data for entity %llu"), EntityID);
		return;
	}

	bool bFoundComponent = false;
	for (const auto& ComponentData : ViewData->Components)
	{
		if (ComponentData.GetComponentId() == PERSISTENCE_COMPONENT_ID)
		{
			ReadSchemaData(*ComponentData.GetFields());
			bFoundComponent = true;
			break;
		}
	}

	if (!bFoundComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Couldn't find the persistence component for entity %llu"), EntityID);
	}
}

void ACustomSchemaSnapshotActor::FillSchemaData(Schema_Object& ComponentObject) const
{
	Schema_AddBool(&ComponentObject, DEPLETED_FIELD_ID, bDepleted);
}

void ACustomSchemaSnapshotActor::ReadSchemaData(Schema_Object& ComponentObject)
{
	bDepleted = Schema_GetBool(&ComponentObject, DEPLETED_FIELD_ID) != 0;
}


