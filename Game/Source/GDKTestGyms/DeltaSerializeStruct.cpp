#include "DeltaSerializeStruct.h"
#include "Net/UnrealNetwork.h"

void FTestStructItem::PostReplicatedChange(const struct FTestStructContainer& InArraySerializer)
{
  int32 ElementIdx = this - InArraySerializer.Items.GetData();
  if (Material)
  {
    int32 ChangeNum = InArraySerializer.Owner->ElementsHavingChanged.FindOrAdd(ElementIdx, 0);
    ++ChangeNum;
  }
}

void FTestStructItem::PreReplicatedRemove(const struct FTestStructContainer& InArraySerializer)
{

}

void FTestStructItem::PostReplicatedAdd(const struct FTestStructContainer& InArraySerializer)
{
  InArraySerializer.Owner->NumInvalidItems = 0;
  int32 ElementIdx = this - InArraySerializer.Items.GetData();

  InArraySerializer.Owner->ElementsHavingChanged.FindOrAdd(ElementIdx, 0);

  for (int i = 0; i< InArraySerializer.Items.Num(); ++i)
  {
    auto const& CurItem = InArraySerializer.Items[i];
    if (!CurItem.Material)
    {
      ++InArraySerializer.Owner->NumInvalidItems;
    }
  }

  InArraySerializer.Owner->ReplicationHappened();
}

bool FTestStructContainer::NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
{
  return FFastArraySerializer::FastArrayDeltaSerialize<FTestStructItem, FTestStructContainer>(Items, DeltaParms, *this);
}

void ANetSerializeTestActor::OnRep_Container()
{
  NumInvalidItems_OnRep = 0;

  for (auto const& Item : TestContainer.Items)
  {
    if (!Item.Material)
    {
      ++NumInvalidItems_OnRep;
    }
  }

  ReplicationHappened();
}

void ANetSerializeTestActor::GetLifetimeReplicatedProps(class TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ANetSerializeTestActor, TestContainer);
}