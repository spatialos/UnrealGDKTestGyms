#include "EventTracerComponent.h"

#include "Interop/Connection/SpatialEventTracerUserInterface.h"
#include "Interop/Connection/SpatialTraceEventBuilder.h"
#include "Net/UnrealNetwork.h"

UEventTracerComponent::UEventTracerComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;

}

void UEventTracerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEventTracerComponent, bTestBool);
}

void UEventTracerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (OwnerHasAuthority() && bUseEventTracing)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEventTracerComponent::TimerFunction, 5.0f, true);
	}
}

void UEventTracerComponent::TimerFunction()
{
	// Create a user trace event for sending a property update
	FString SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.send_component_property").GetEvent());
	USpatialEventTracerUserInterface::AddLatentSpanId(this, this, SpanId);

	bTestBool = !bTestBool;

	// Create a user trace event for sending an RPC
	SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.send_rpc").GetEvent());

	USpatialEventTracerUserInterface::AddSpanIdToStack(this, SpanId);
	RunOnClient();
	USpatialEventTracerUserInterface::PopSpanIdFromStack(this);
}

void UEventTracerComponent::OnRepBool()
{
	if (!OwnerHasAuthority())
	{
		// Create a user trace event for receiving a property update
		FString SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
		USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.receive_component_property").GetEvent());
	}
}

void UEventTracerComponent::RunOnClient_Implementation()
{
	// Create a user trace event for receiving an RPC
	FString SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.process_rpc").GetEvent());
}

bool UEventTracerComponent::OwnerHasAuthority() const
{
	AActor* Owner = GetOwner();
	return Owner != nullptr && Owner->HasAuthority();
}