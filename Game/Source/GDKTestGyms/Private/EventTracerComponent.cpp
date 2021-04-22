#include "EventTracerComponent.h"

#include "Interop/Connection/SpatialEventTracerUserInterface.h"
#include "Interop/Connection/UserSpanId.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UEventTracerComponent::UEventTracerComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

void UEventTracerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEventTracerComponent, TestInt);
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
	FUserSpanId SpanId;// = USpatialEventTracerUserInterface::TraceEvent(this, SpatialGDK::FSpatialTraceEventBuilder("user.send_component_property").GetEvent());
	USpatialEventTracerUserInterface::TraceProperty(this, this, SpanId);

	TestInt++;

	// Create a user trace event for sending an RPC
	SpanId;//= USpatialEventTracerUserInterface::TraceEvent(this, SpatialGDK::FSpatialTraceEventBuilder("user.send_rpc").GetEvent());

	FEventTracerRPCDelegate Delegate;
	Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UEventTracerComponent, RunOnClient));
	USpatialEventTracerUserInterface::TraceRPC(this, Delegate, SpanId);
}

void UEventTracerComponent::OnRepTestInt()
{
	if (!OwnerHasAuthority())
	{
		FUserSpanId CauseSpanId;
		if (USpatialEventTracerUserInterface::GetActiveSpanId(this, CauseSpanId))
		{
			;//USpatialEventTracerUserInterface::TraceEventWithCauses(this, SpatialGDK::FSpatialTraceEventBuilder("user.receive_component_property").GetEvent(), { CauseSpanId });
		}
	}
}

void UEventTracerComponent::RunOnClient_Implementation()
{
	// Create a user trace event for receiving an RPC

	FUserSpanId CauseSpanId;
	if (USpatialEventTracerUserInterface::GetActiveSpanId(this, CauseSpanId))
	{
		;//USpatialEventTracerUserInterface::TraceEventWithCauses(this, SpatialGDK::FSpatialTraceEventBuilder("user.process_rpc").GetEvent(), { CauseSpanId });
	}
}

bool UEventTracerComponent::OwnerHasAuthority() const
{
	AActor* Owner = GetOwner();
	return Owner != nullptr && Owner->HasAuthority();
}