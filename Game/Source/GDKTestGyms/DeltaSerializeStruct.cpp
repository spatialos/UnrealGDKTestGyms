#include "DeltaSerializeStruct.h"
#include "Net/UnrealNetwork.h"

void FTestStructContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	PostReplicatedAdd(ChangedIndices, FinalSize);
}

void FTestStructContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
}

void FTestStructContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	Owner->NumInvalidItems = 0;

	for (const auto& CurItem : Items)
	{
		if (CurItem.Material == nullptr)
		{
			++Owner->NumInvalidItems;
		}
	}

	Owner->ReplicationHappened();
}

bool FTestStructContainer::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FTestStructItem, FTestStructContainer>(Items, DeltaParms, *this);
}

void ANetSerializeTestActor::OnRep_Container()
{
	NumInvalidItems_OnRep = 0;

	for (auto const& Item : TestContainer.Items)
	{
		if (Item.Material == nullptr)
		{
			++NumInvalidItems_OnRep;
		}
	}

	ReplicationHappened();
}

void ANetSerializeTestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetSerializeTestActor, TestContainer);
}
