#include "EventTracerComponent.h"

#include "Interop/Connection/SpatialEventTracerUserInterface.h"
#include "Interop/Connection/SpatialTraceEventBuilder.h"
#include "Interop/Connection/UserSpanId.h"
#include "Net/UnrealNetwork.h"

UEventTracerComponent::UEventTracerComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;

}

void UEventTracerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEventTracerComponent, bTestInt);
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
	FUserSpanId SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.send_component_property").GetEvent());
	USpatialEventTracerUserInterface::AddLatentSpanId(this, this, SpanId);

	bTestInt++;

	// Create a user trace event for sending an RPC
	SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.send_rpc").GetEvent());

	FEventTracerDynamicDelegate Delegate;
	Delegate.BindUFunction(this, "RunOnClient");
	USpatialEventTracerUserInterface::SetActiveSpanId(this, Delegate, SpanId);
}

void UEventTracerComponent::OnRepBool()
{
	if (!OwnerHasAuthority())
	{
		FUserSpanId SpanId;
		if (USpatialEventTracerUserInterface::GetActiveSpanId(this, SpanId))
		{
			FUserSpanId SpanId = USpatialEventTracerUserInterface::CreateSpanIdWithCauses(this, { SpanId });
			USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.receive_component_property").GetEvent());
		}
	}
}

void UEventTracerComponent::RunOnClient_Implementation()
{
	// Create a user trace event for receiving an RPC

	FUserSpanId SpanId;
	if (USpatialEventTracerUserInterface::GetActiveSpanId(this, SpanId))
	{
		FUserSpanId SpanId = USpatialEventTracerUserInterface::CreateSpanIdWithCauses(this, { SpanId });
		USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder("user.process_rpc").GetEvent());
	}
}

bool UEventTracerComponent::OwnerHasAuthority() const
{
	AActor* Owner = GetOwner();
	return Owner != nullptr && Owner->HasAuthority();
}