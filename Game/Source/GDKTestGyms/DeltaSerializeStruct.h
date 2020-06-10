#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "DeltaSerializeStruct.generated.h"

struct FTestStructContainer;

USTRUCT(BlueprintType)
struct FTestStructItem : public FFastArraySerializerItem
{
  GENERATED_USTRUCT_BODY()

    FTestStructItem()
  {}
  
  UPROPERTY(BlueprintReadWrite)
    UMaterial* Material;

  void PreReplicatedRemove(const struct FTestStructContainer& InArraySerializer);
  void PostReplicatedAdd(const struct FTestStructContainer& InArraySerializer);
  void PostReplicatedChange(const struct FTestStructContainer& InArraySerializer);
};

class ANetSerializeTestActor;

/** Fast serializer wrapper for above struct */
USTRUCT(BlueprintType)
struct FTestStructContainer : public FFastArraySerializer
{
  GENERATED_USTRUCT_BODY()

    FTestStructContainer()
  {}

    FTestStructContainer(ANetSerializeTestActor* InOwner)
    : Owner(InOwner)
  {
  }

  UPROPERTY()
    ANetSerializeTestActor* Owner;

  UPROPERTY(BlueprintReadWrite)
    TArray<FTestStructItem> Items;

  bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms);
};

template<>
struct TStructOpsTypeTraits< FTestStructContainer > : public TStructOpsTypeTraitsBase2< FTestStructContainer >
{
  enum
  {
    WithNetDeltaSerializer = true,
  };
};

UCLASS(BlueprintType)
class ANetSerializeTestActor : public AActor 
{
  friend FTestStructItem;
  friend FTestStructContainer;
public:
  GENERATED_BODY()

  ANetSerializeTestActor()
    : TestContainer(this)
  {
    bReplicates = true;
  }

  UFUNCTION(BlueprintCallable)
    void AddMaterial(UMaterial* InMaterial)
  {
    TestContainer.Items.AddDefaulted();
    TestContainer.Items.Last().Material = InMaterial;
    TestContainer.MarkArrayDirty();
  }

  UFUNCTION(BlueprintCallable)
    void SetMaterial(int32 Index, UMaterial* InMaterial) 
  { 
    TestContainer.Items[Index].Material = InMaterial; 
    TestContainer.MarkItemDirty(TestContainer.Items[Index]);
  }

protected:

  void GetLifetimeReplicatedProps(class TArray<class FLifetimeProperty> &) const;

  UFUNCTION()
  void OnRep_Container();

  UFUNCTION(BlueprintImplementableEvent)
  void ReplicationHappened();

  UPROPERTY(BlueprintReadOnly)
    int32 NumItems = 0;

  UPROPERTY(BlueprintReadOnly)
    TMap<int32, int32> ElementsHavingChanged;

  UPROPERTY(BlueprintReadOnly)
    int32 NumInvalidItems = 0;
  UPROPERTY(BlueprintReadOnly)
    int32 NumInvalidItems_OnRep = 0;

  UPROPERTY(ReplicatedUsing = OnRep_Container, EditAnywhere, BlueprintReadWrite)
    FTestStructContainer TestContainer;
};