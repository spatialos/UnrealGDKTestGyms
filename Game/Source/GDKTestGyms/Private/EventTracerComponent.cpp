#include "EventTracerComponent.h"

#include "Interop/Connection/SpatialEventTracerUserInterface.h"
#include "Interop/Connection/SpatialTraceEventBuilder.h"
#include "Interop/Connection/UserSpanId.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "SpatialGDKFunctionalTests/SpatialGDK/EventTracingTests/EventTracingTestConstants.h"

UEventTracerComponent::UEventTracerComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

void UEventTracerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEventTracerComponent, TestInt);
}

void UEventTracerComponent::BeginPlay()
{
	Super::BeginPlay();

	UserSendRPCEventName = UEventTracingTestConstants::GetUserSendRPCEventName().ToString();
	UserProcessRPCEventName = UEventTracingTestConstants::GetUserProcessRPCEventName().ToString();
	UserSendComponentPropertyEventName = UEventTracingTestConstants::GetUserSendComponentPropertyEventName().ToString();
	UserReceiveComponentPropertyEventName = UEventTracingTestConstants::GetUserReceiveComponentPropertyEventName().ToString();

	if (OwnerHasAuthority() && bUseEventTracing)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEventTracerComponent::TimerFunction, 5.0f, true);
	}
}

void UEventTracerComponent::TimerFunction()
{
	// Create a user trace event for sending a property update
	FUserSpanId SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder(UserSendComponentPropertyEventName).GetEvent());
	USpatialEventTracerUserInterface::TraceProperty(this, this, SpanId);

	TestInt++;

	// Create a user trace event for sending an RPC
	SpanId = USpatialEventTracerUserInterface::CreateSpanId(this);
	USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder(UserSendRPCEventName).GetEvent());

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
			FUserSpanId SpanId = USpatialEventTracerUserInterface::CreateSpanIdWithCauses(this, { CauseSpanId });
			USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder(UserReceiveComponentPropertyEventName).GetEvent());
		}
	}
}

void UEventTracerComponent::RunOnClient_Implementation()
{
	// Create a user trace event for receiving an RPC

	FUserSpanId CauseSpanId;
	if (USpatialEventTracerUserInterface::GetActiveSpanId(this, CauseSpanId))
	{
		FUserSpanId SpanId = USpatialEventTracerUserInterface::CreateSpanIdWithCauses(this, { CauseSpanId });
		USpatialEventTracerUserInterface::TraceEvent(this, SpanId, SpatialGDK::FSpatialTraceEventBuilder(UserProcessRPCEventName).GetEvent());
	}
}

bool UEventTracerComponent::OwnerHasAuthority() const
{
	AActor* Owner = GetOwner();
	return Owner != nullptr && Owner->HasAuthority();
}