// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

/**
*
*/

#include "TestGymsReplicationGraph.h"

#include "CoreGlobals.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Launch/Resources/Version.h"

#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/LevelScriptActor.h"

DEFINE_LOG_CATEGORY(LogTestGymsReplicationGraph);

int32 CVar_TestGymsRepGraph_DisableSpatialRebuilds = 1;
static FAutoConsoleVariableRef CVarTestGymsRepDisableSpatialRebuilds(TEXT("TestGymsRepGraph.DisableSpatialRebuilds"), CVar_TestGymsRepGraph_DisableSpatialRebuilds, TEXT(""), ECVF_Default);

// ----------------------------------------------------------------------------------------------------------


UTestGymsReplicationGraph::UTestGymsReplicationGraph()
{
	static ConstructorHelpers::FClassFinder<AActor> ReplicatedBP(TEXT("/Game/Actors/ReplicatedActor"));
	if (ReplicatedBP.Class != nullptr)
	{
		ReplicatedBPClass = ReplicatedBP.Class;
	}
}

void InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool bSpatialize, float ServerMaxTickRate)
{
	AActor* CDO = Class->GetDefaultObject<AActor>();
	if (bSpatialize)
	{
		Info.SetCullDistanceSquared(CDO->NetCullDistanceSquared);
		UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("Setting cull distance for %s to %f (%f)"), *Class->GetName(), Info.GetCullDistanceSquared(), Info.GetCullDistance());
	}

	Info.ReplicationPeriodFrame = FMath::Max<uint32>((uint32)FMath::RoundToFloat(ServerMaxTickRate / CDO->NetUpdateFrequency), 1);

	UClass* NativeClass = Class;
	while (!NativeClass->IsNative() && NativeClass->GetSuperClass() && NativeClass->GetSuperClass() != AActor::StaticClass())
	{
		NativeClass = NativeClass->GetSuperClass();
	}

	UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("Setting replication period for %s (%s) to %d frames (%.2f)"), *Class->GetName(), *NativeClass->GetName(), Info.ReplicationPeriodFrame, CDO->NetUpdateFrequency);
}

void UTestGymsReplicationGraph::ResetGameWorldState()
{
	Super::ResetGameWorldState();

	AlwaysRelevantStreamingLevelActors.Empty();

	for (UNetReplicationGraphConnection* ConnManager : Connections)
	{
		for (UReplicationGraphNode* ConnectionNode : ConnManager->GetConnectionGraphNodes())
		{
			if (UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
			{
				AlwaysRelevantConnectionNode->ResetGameWorldState();
			}
		}
	}

	for (UNetReplicationGraphConnection* ConnManager : PendingConnections)
	{
		for (UReplicationGraphNode* ConnectionNode : ConnManager->GetConnectionGraphNodes())
		{
			if (UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
			{
				AlwaysRelevantConnectionNode->ResetGameWorldState();
			}
		}
	}
}

void UTestGymsReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	const bool bUsingSpatial = GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking();

	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Programatically build the rules.
	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	auto AddInfo = [&](UClass* Class, EClassRepNodeMapping Mapping) { ClassRepNodePolicies.Set(Class, Mapping); };

	AddInfo(APlayerState::StaticClass(), EClassRepNodeMapping::NotRouted);				// Special cased via UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter
	AddInfo(AReplicationGraphDebugActor::StaticClass(), EClassRepNodeMapping::NotRouted);				// Not supported
	AddInfo(AInfo::StaticClass(), EClassRepNodeMapping::RelevantAllConnections);	// Non spatialized, relevant to all
	AddInfo(ReplicatedBPClass, EClassRepNodeMapping::Spatialize_Dynamic);		// Add our replicated base class to ensure we don't miss out-of-memory bp classes

	if (bUsingSpatial)
	{
		// Game mode is replicated in spatial, ensure it is always replicated
		AddInfo(AGameModeBase::StaticClass(), EClassRepNodeMapping::RelevantAllConnections);
		// Add always replicated test actor. Use soft class path to work around module dependencies.
		FSoftClassPath SoftClassPath(TEXT("Class'/Script/SpatialGDKFunctionalTests.ReplicatedTestActorBase_RepGraph'"));
		if (UClass* Class = SoftClassPath.ResolveClass())
		{
			AddInfo(Class, EClassRepNodeMapping::AlwaysReplicate);
		}
	}

	TArray<UClass*> AllReplicatedClasses;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (!ActorCDO || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		// Skip SKEL and REINST classes.
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		if (bUsingSpatial && !Class->HasAnySpatialClassFlags(SPATIALCLASS_SpatialType))
		{
			// Anything not added to ClassRepNodePolicies will default to NotRouted
			continue;
		}

		// --------------------------------------------------------------------
		// This is a replicated class. Save this off for the second pass below
		// --------------------------------------------------------------------

		AllReplicatedClasses.Add(Class);

		// Skip if already in the map (added explicitly)
		if (ClassRepNodePolicies.Contains(Class, false))
		{
			continue;
		}

		auto ShouldSpatialize = [](const AActor* CDO)
		{
			return CDO->GetIsReplicated() && (!(CDO->bAlwaysRelevant || CDO->bOnlyRelevantToOwner || CDO->bNetUseOwnerRelevancy));
		};

		auto GetLegacyDebugStr = [](const AActor* CDO)
		{
			return FString::Printf(TEXT("%s [%d/%d/%d]"), *CDO->GetClass()->GetName(), CDO->bAlwaysRelevant, CDO->bOnlyRelevantToOwner, CDO->bNetUseOwnerRelevancy);
		};

		// Only handle this class if it differs from its super. There is no need to put every child class explicitly in the graph class mapping
		UClass* SuperClass = Class->GetSuperClass();
		if (AActor* SuperCDO = Cast<AActor>(SuperClass->GetDefaultObject()))
		{
			if (SuperCDO->GetIsReplicated() == ActorCDO->GetIsReplicated()
				&& SuperCDO->bAlwaysRelevant == ActorCDO->bAlwaysRelevant
				&& SuperCDO->bOnlyRelevantToOwner == ActorCDO->bOnlyRelevantToOwner
				&& SuperCDO->bNetUseOwnerRelevancy == ActorCDO->bNetUseOwnerRelevancy
				)
			{
				continue;
			}

			if (ShouldSpatialize(ActorCDO) == false && ShouldSpatialize(SuperCDO) == true)
			{
				UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("Adding %s to NonSpatializedChildClasses. (Parent: %s)"), *GetLegacyDebugStr(ActorCDO), *GetLegacyDebugStr(SuperCDO));
				NonSpatializedChildClasses.Add(Class);
			}
		}

		if (ShouldSpatialize(ActorCDO))
		{
			AddInfo(Class, EClassRepNodeMapping::Spatialize_Dynamic);
		}
		else if (ActorCDO->bAlwaysRelevant && (!ActorCDO->bOnlyRelevantToOwner || bUsingSpatial))
		{
			AddInfo(Class, EClassRepNodeMapping::RelevantAllConnections);
		}
		else
		{
			UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("Not adding info for class %s."), *GetLegacyDebugStr(ActorCDO));
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Setup FClassReplicationInfo. This is essentially the per class replication settings. Some we set explicitly, the rest we are setting via looking at the legacy settings on AActor.
	// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	TArray<UClass*> ExplicitlySetClasses;
	auto SetClassInfo = [&](UClass* Class, const FClassReplicationInfo& Info) { GlobalActorReplicationInfoMap.SetClassInfo(Class, Info); ExplicitlySetClasses.Add(Class); };

	/*
	// Example of overriding APawn class info
	FClassReplicationInfo PawnClassRepInfo;
	PawnClassRepInfo.DistancePriorityScale = 1.f;
	PawnClassRepInfo.StarvationPriorityScale = 1.f;
	PawnClassRepInfo.ActorChannelFrameTimeout = 4;
	PawnClassRepInfo.SetCullDistanceSquared(15000.f * 15000.f); // Yuck
	SetClassInfo( APawn::StaticClass(), PawnClassRepInfo );
	*/

	FClassReplicationInfo PlayerStateRepInfo;
	PlayerStateRepInfo.DistancePriorityScale = 0.f;
	PlayerStateRepInfo.ActorChannelFrameTimeout = 0;
	SetClassInfo(APlayerState::StaticClass(), PlayerStateRepInfo);

	UReplicationGraphNode_ActorListFrequencyBuckets::DefaultSettings.ListSize = 12;

	// Set FClassReplicationInfo based on legacy settings from all replicated classes
	for (UClass* ReplicatedClass : AllReplicatedClasses)
	{
		if (ExplicitlySetClasses.FindByPredicate([&](const UClass* SetClass) { return ReplicatedClass->IsChildOf(SetClass); }) != nullptr)
		{
			continue;
		}

		const bool bClassIsSpatialized = IsSpatialized(ClassRepNodePolicies.GetChecked(ReplicatedClass));

		FClassReplicationInfo ClassInfo;
		InitClassReplicationInfo(ClassInfo, ReplicatedClass, bClassIsSpatialized, NetDriver->NetServerMaxTickRate);
		GlobalActorReplicationInfoMap.SetClassInfo(ReplicatedClass, ClassInfo);
	}


	// Print out what we came up with
	UE_LOG(LogTestGymsReplicationGraph, Log, TEXT(""));
	UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("Class Routing Map: "));
	UEnum* Enum = StaticEnum<EClassRepNodeMapping>();
	for (auto ClassMapIt = ClassRepNodePolicies.CreateIterator(); ClassMapIt; ++ClassMapIt)
	{
		UClass* Class = CastChecked<UClass>(ClassMapIt.Key().ResolveObjectPtr());
		const EClassRepNodeMapping Mapping = ClassMapIt.Value();

		// Only print if different than native class
		UClass* ParentNativeClass = GetParentNativeClass(Class);
		const EClassRepNodeMapping* ParentMapping = ClassRepNodePolicies.Get(ParentNativeClass);
		if (ParentMapping && Class != ParentNativeClass && Mapping == *ParentMapping)
		{
			continue;
		}

		UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("  %s (%s) -> %s"), *Class->GetName(), *GetNameSafe(ParentNativeClass), *Enum->GetNameStringByValue(static_cast<uint32>(Mapping)));
	}

	UE_LOG(LogTestGymsReplicationGraph, Log, TEXT(""));
	UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("Class Settings Map: "));
	FClassReplicationInfo DefaultValues;
	for (auto ClassRepInfoIt = GlobalActorReplicationInfoMap.CreateClassMapIterator(); ClassRepInfoIt; ++ClassRepInfoIt)
	{
		UClass* Class = CastChecked<UClass>(ClassRepInfoIt.Key().ResolveObjectPtr());
		const FClassReplicationInfo& ClassInfo = ClassRepInfoIt.Value();
		UE_LOG(LogTestGymsReplicationGraph, Log, TEXT("  %s (%s) -> %s"), *Class->GetName(), *GetNameSafe(GetParentNativeClass(Class)), *ClassInfo.BuildDebugStringDelta());
	}

	// -------------------------------------------------------
	//	Register for game code callbacks.
	//	This could have been done the other way: E.g, AMyGameActor could do GetNetDriver()->GetReplicationDriver<UTestGymsReplicationGraph>()->OnMyGameEvent etc.
	//	This way at least keeps the rep graph out of game code directly and allows rep graph to exist in its own module
	//	So for now, erring on the side of a cleaning dependencies between classes.
	// -------------------------------------------------------
}

void UTestGymsReplicationGraph::InitGlobalGraphNodes()
{
	// Preallocate some replication lists.
	PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(128, 64);
	PreAllocateRepList(512, 16);

	// -----------------------------------------------
	//	Spatial Actors
	// -----------------------------------------------

	GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>();
	GridNode->CellSize = 10000.f;
	GridNode->SpatialBias = FVector2D(-WORLD_MAX, -WORLD_MAX);

	if (CVar_TestGymsRepGraph_DisableSpatialRebuilds)
	{
		GridNode->AddSpatialRebuildBlacklistClass(AActor::StaticClass()); // Disable All spatial rebuilding
	}

	GridNode->SetProcessOnSpatialConnectionOnly();
	AddGlobalGraphNode(GridNode);

	// -----------------------------------------------
	//	Always Relevant (to everyone) Actors
	// -----------------------------------------------
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AlwaysRelevantNode->SetProcessOnSpatialConnectionOnly();
	AddGlobalGraphNode(AlwaysRelevantNode);

	// -----------------------------------------------
	//	Player State specialization. This will return a rolling subset of the player states to replicate
	// -----------------------------------------------
	UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter* PlayerStateNode = CreateNewNode<UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter>();
	PlayerStateNode->SetProcessOnSpatialConnectionOnly();
	AddGlobalGraphNode(PlayerStateNode);

	if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		// -----------------------------------------------
		//	Ensure every connections view/target gets replicated each frame. This is handled per connection in native in UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection
		// -----------------------------------------------
		UTestGymsReplicationGraphNode_GlobalViewTarget* ViewTargetNode = CreateNewNode<UTestGymsReplicationGraphNode_GlobalViewTarget>();
		ViewTargetNode->SetProcessOnSpatialConnectionOnly();
		AddGlobalGraphNode(ViewTargetNode);
	}
}

void UTestGymsReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
	Super::InitConnectionGraphNodes(RepGraphConnection);

	if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		// We don't need a per-connection always relevancy node in spatial
		return;
	}

	UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = CreateNewNode<UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection>();

	// This node needs to know when client levels go in and out of visibility
	RepGraphConnection->OnClientVisibleLevelNameAdd.AddUObject(AlwaysRelevantConnectionNode, &UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd);
	RepGraphConnection->OnClientVisibleLevelNameRemove.AddUObject(AlwaysRelevantConnectionNode, &UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove);

	AddConnectionGraphNode(AlwaysRelevantConnectionNode, RepGraphConnection);
}

EClassRepNodeMapping UTestGymsReplicationGraph::GetMappingPolicy(UClass* Class)
{
	EClassRepNodeMapping* PolicyPtr = ClassRepNodePolicies.Get(Class);
	EClassRepNodeMapping Policy = PolicyPtr ? *PolicyPtr : EClassRepNodeMapping::NotRouted;
	return Policy;
}

void UTestGymsReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo)
{
	const bool bUsingSpatial = GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking();
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	switch (Policy)
	{
	case EClassRepNodeMapping::NotRouted:
	{
		UE_LOG(LogTestGymsReplicationGraph, Verbose, TEXT("RouteAddNetworkActorToNodes: Not Routed - %s"), *GetNameSafe(ActorInfo.GetActor()));
		break;
	}

	case EClassRepNodeMapping::AlwaysReplicate:
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
		break;
	}

	case EClassRepNodeMapping::RelevantAllConnections:
	{
		// When running in Spatial, we don't need to handle per-connection level relevancy, as the runtime takes care of interest management for us
		if (ActorInfo.StreamingLevelName == NAME_None || bUsingSpatial)
		{
			AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
		}
		else
		{
			FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindOrAdd(ActorInfo.StreamingLevelName);
			RepList.PrepareForWrite();
			RepList.ConditionalAdd(ActorInfo.Actor);
		}
		break;
	}

	case EClassRepNodeMapping::Spatialize_Static:
	{
		GridNode->AddActor_Static(ActorInfo, GlobalInfo);
		break;
	}

	case EClassRepNodeMapping::Spatialize_Dynamic:
	{
		GridNode->AddActor_Dynamic(ActorInfo, GlobalInfo);
		break;
	}

	case EClassRepNodeMapping::Spatialize_Dormancy:
	{
		GridNode->AddActor_Dormancy(ActorInfo, GlobalInfo);
		break;
	}
	};
}

void UTestGymsReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	const bool bUsingSpatial = GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking();
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	switch (Policy)
	{
	case EClassRepNodeMapping::NotRouted:
	{
		break;
	}

	case EClassRepNodeMapping::AlwaysReplicate:
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
		break;
	}

	case EClassRepNodeMapping::RelevantAllConnections:
	{
		// When running in Spatial, we don't need to handle per-connection level relevancy, as the runtime takes care of interest management for us
		if (ActorInfo.StreamingLevelName == NAME_None || bUsingSpatial)
		{
			AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
		}
		else
		{
			FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindChecked(ActorInfo.StreamingLevelName);
			if (RepList.Remove(ActorInfo.Actor) == false)
			{
				UE_LOG(LogTestGymsReplicationGraph, Warning, TEXT("Actor %s was not found in AlwaysRelevantStreamingLevelActors list. LevelName: %s"), *GetActorRepListTypeDebugString(ActorInfo.Actor), *ActorInfo.StreamingLevelName.ToString());
			}
		}
		break;
	}

	case EClassRepNodeMapping::Spatialize_Static:
	{
		GridNode->RemoveActor_Static(ActorInfo);
		break;
	}

	case EClassRepNodeMapping::Spatialize_Dynamic:
	{
		GridNode->RemoveActor_Dynamic(ActorInfo);
		break;
	}

	case EClassRepNodeMapping::Spatialize_Dormancy:
	{
		GridNode->RemoveActor_Dormancy(ActorInfo);
		break;
	}
	};
}

// ------------------------------------------------------------------------------

void UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::ResetGameWorldState()
{
	AlwaysRelevantStreamingLevelsNeedingReplication.Empty();
}

void UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	QUICK_SCOPE_CYCLE_COUNTER(UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection_GatherActorListsForConnection);

	UTestGymsReplicationGraph* TestGymsGraph = CastChecked<UTestGymsReplicationGraph>(GetOuter());

	ReplicationActorList.Reset();

	auto ResetActorCullDistance = [&](AActor* ActorToSet, AActor*& LastActor) {

		if (ActorToSet != LastActor)
		{
			LastActor = ActorToSet;

			UE_LOG(LogTestGymsReplicationGraph, Verbose, TEXT("Setting pawn cull distance to 0. %s"), *ActorToSet->GetName());
			FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd(ActorToSet);
			ConnectionActorInfo.SetCullDistanceSquared(0.f);
		}
	};

	for (const FNetViewer& CurViewer : Params.Viewers)
	{
		ReplicationActorList.ConditionalAdd(CurViewer.InViewer);
		ReplicationActorList.ConditionalAdd(CurViewer.ViewTarget);

		if (APlayerController* PC = Cast<APlayerController>(CurViewer.InViewer))
		{
			// 50% throttling of PlayerStates.
#if ENGINE_MINOR_VERSION >= 26
			const bool bReplicatePS = (Params.ConnectionManager.ConnectionOrderNum % 2) == (Params.ReplicationFrameNum % 2);
#else
			const bool bReplicatePS = (Params.ConnectionManager.ConnectionId % 2) == (Params.ReplicationFrameNum % 2);
#endif
			if (bReplicatePS)
			{
				// Always return the player state to the owning player. Simulated proxy player states are handled by UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter
				if (APlayerState* PS = PC->PlayerState)
				{
					if (!bInitializedPlayerState)
					{
						bInitializedPlayerState = true;
						FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd(PS);
						ConnectionActorInfo.ReplicationPeriodFrame = 1;
					}

					ReplicationActorList.ConditionalAdd(PS);
				}
			}

			FAlwaysRelevantActorInfo* LastData = PastRelevantActors.FindByKey<UNetConnection*>(CurViewer.Connection);

			// We've not seen this actor before, go ahead and add them.
			if (LastData == nullptr)
			{
				FAlwaysRelevantActorInfo NewActorInfo;
				NewActorInfo.Connection = CurViewer.Connection;
				LastData = &(PastRelevantActors[PastRelevantActors.Add(NewActorInfo)]);
			}

			check(LastData != nullptr);

			if (ACharacter* Pawn = Cast<ACharacter>(PC->GetPawn()))
			{
				ResetActorCullDistance(Pawn, LastData->LastViewer);

				if (Pawn != CurViewer.ViewTarget)
				{
					ReplicationActorList.ConditionalAdd(Pawn);
				}
			}

			if (ACharacter* ViewTargetPawn = Cast<ACharacter>(CurViewer.ViewTarget))
			{
				ResetActorCullDistance(ViewTargetPawn, LastData->LastViewTarget);
			}
		}
	}

	PastRelevantActors.RemoveAll([&](FAlwaysRelevantActorInfo& RelActorInfo) {
		return RelActorInfo.Connection == nullptr;
	});

	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);

	// Always relevant streaming level actors.
	FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;

	TMap<FName, FActorRepListRefView>& AlwaysRelevantStreamingLevelActors = TestGymsGraph->AlwaysRelevantStreamingLevelActors;

	for (int32 Idx = AlwaysRelevantStreamingLevelsNeedingReplication.Num() - 1; Idx >= 0; --Idx)
	{
		const FName& StreamingLevel = AlwaysRelevantStreamingLevelsNeedingReplication[Idx];

		FActorRepListRefView* Ptr = AlwaysRelevantStreamingLevelActors.Find(StreamingLevel);
		if (Ptr == nullptr)
		{
			// No always relevant lists for that level
			AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			continue;
		}

		FActorRepListRefView& RepList = *Ptr;

		if (RepList.Num() > 0)
		{
			bool bAllDormant = true;
			for (FActorRepListType Actor : RepList)
			{
				FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionActorInfoMap.FindOrAdd(Actor);
				if (ConnectionActorInfo.bDormantOnConnection == false)
				{
					bAllDormant = false;
					break;
				}
			}

			if (bAllDormant)
			{
				AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			}
			else
			{
				Params.OutGatheredReplicationLists.AddReplicationActorList(RepList);
			}
		}
		else
		{
			UE_LOG(LogTestGymsReplicationGraph, Warning, TEXT("UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection - empty RepList %s"), *Params.ConnectionManager.GetName());
		}

	}
}

void UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd(FName LevelName, UWorld* StreamingWorld)
{
	AlwaysRelevantStreamingLevelsNeedingReplication.Add(LevelName);
}

void UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove(FName LevelName)
{
	AlwaysRelevantStreamingLevelsNeedingReplication.Remove(LevelName);
}

void UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection::LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const
{
	DebugInfo.Log(NodeName);
	DebugInfo.PushIndent();
	LogActorRepList(DebugInfo, NodeName, ReplicationActorList);

	for (const FName& LevelName : AlwaysRelevantStreamingLevelsNeedingReplication)
	{
		UTestGymsReplicationGraph* TestGymsGraph = CastChecked<UTestGymsReplicationGraph>(GetOuter());
		if (FActorRepListRefView* RepList = TestGymsGraph->AlwaysRelevantStreamingLevelActors.Find(LevelName))
		{
			LogActorRepList(DebugInfo, FString::Printf(TEXT("AlwaysRelevant StreamingLevel List: %s"), *LevelName.ToString()), *RepList);
		}
	}

	DebugInfo.PopIndent();
}

// ------------------------------------------------------------------------------

UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter::UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter()
{
	bRequiresPrepareForReplicationCall = true;

	if (GetDefault<UGeneralProjectSettings>()->UsesSpatialNetworking())
	{
		// Increase number of player states we replicate per frame when running in Spatial as we don't get the benefit of
		// UTestGymsReplicationGraphNode_AlwaysRelevant_ForConnection ensuring that the owning connection's PlayerState is replicated every frame
		TargetActorsPerFrame = 16;
	}
}

void UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter::PrepareForReplication()
{
	QUICK_SCOPE_CYCLE_COUNTER(UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter_GlobalPrepareForReplication);

	ReplicationActorLists.Reset();
	ForceNetUpdateReplicationActorList.Reset();

	ReplicationActorLists.AddDefaulted();
	FActorRepListRefView* CurrentList = &ReplicationActorLists[0];
	CurrentList->PrepareForWrite();

	// We rebuild our lists of player states each frame. This is not as efficient as it could be but its the simplest way
	// to handle players disconnecting and keeping the lists compact. If the lists were persistent we would need to defrag them as players left.

	for (TActorIterator<APlayerState> It(GetWorld()); It; ++It)
	{
		APlayerState* PS = *It;
		if (IsActorValidForReplicationGather(PS) == false)
		{
			continue;
		}

		if (CurrentList->Num() >= TargetActorsPerFrame)
		{
			ReplicationActorLists.AddDefaulted();
			CurrentList = &ReplicationActorLists.Last();
			CurrentList->PrepareForWrite();
		}

		CurrentList->Add(PS);
	}
}

void UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	const int32 ListIdx = Params.ReplicationFrameNum % ReplicationActorLists.Num();
	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorLists[ListIdx]);

	if (ForceNetUpdateReplicationActorList.Num() > 0)
	{
		Params.OutGatheredReplicationLists.AddReplicationActorList(ForceNetUpdateReplicationActorList);
	}
}

void UTestGymsReplicationGraphNode_PlayerStateFrequencyLimiter::LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const
{
	DebugInfo.Log(NodeName);
	DebugInfo.PushIndent();

	int32 i = 0;
	for (const FActorRepListRefView& List : ReplicationActorLists)
	{
		LogActorRepList(DebugInfo, FString::Printf(TEXT("Bucket[%d]"), i++), List);
	}

	DebugInfo.PopIndent();
}

void UTestGymsReplicationGraphNode_GlobalViewTarget::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	ReplicationActorList.Reset();

	for (const FNetViewer& CurViewer : Params.Viewers)
	{
		if (CurViewer.InViewer)
		{
			ReplicationActorList.ConditionalAdd(CurViewer.InViewer);
		}

		if (CurViewer.ViewTarget)
		{
			ReplicationActorList.ConditionalAdd(CurViewer.ViewTarget);
		}

		if (APlayerController* PC = Cast<APlayerController>(CurViewer.InViewer))
		{
			if (ACharacter* Pawn = Cast<ACharacter>(PC->GetPawn()))
			{
				if (Pawn != CurViewer.ViewTarget)
				{
					ReplicationActorList.ConditionalAdd(Pawn);
				}
			}
		}
	}

	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
}

void UTestGymsReplicationGraphNode_GlobalViewTarget::LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const
{
	DebugInfo.Log(NodeName);
	DebugInfo.PushIndent();
	LogActorRepList(DebugInfo, NodeName, ReplicationActorList);
	DebugInfo.PopIndent();
}

// ------------------------------------------------------------------------------

void UTestGymsReplicationGraph::PrintRepNodePolicies()
{
	UEnum* Enum = StaticEnum<EClassRepNodeMapping>();
	if (!Enum)
	{
		return;
	}

	GLog->Logf(TEXT("===================================="));
	GLog->Logf(TEXT("TestGyms Replication Routing Policies"));
	GLog->Logf(TEXT("===================================="));

	for (auto It = ClassRepNodePolicies.CreateIterator(); It; ++It)
	{
		FObjectKey ObjKey = It.Key();

		EClassRepNodeMapping Mapping = It.Value();

		GLog->Logf(TEXT("%-40s --> %s"), *GetNameSafe(ObjKey.ResolveObjectPtr()), *Enum->GetNameStringByValue(static_cast<uint32>(Mapping)));
	}
}

FAutoConsoleCommandWithWorldAndArgs TestGymsPrintRepNodePoliciesCmd(TEXT("TestGymsRepGraph.PrintRouting"), TEXT("Prints how actor classes are routed to RepGraph nodes"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
{
	for (TObjectIterator<UTestGymsReplicationGraph> It; It; ++It)
	{
		It->PrintRepNodePolicies();
	}
})
);

// ------------------------------------------------------------------------------

FAutoConsoleCommandWithWorldAndArgs ChangeFrequencyBucketsCmd(TEXT("TestGymsRepGraph.FrequencyBuckets"), TEXT("Resets frequency bucket count."), FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray< FString >& Args, UWorld* World)
{
	int32 Buckets = 1;
	if (Args.Num() > 0)
	{
		LexTryParseString<int32>(Buckets, *Args[0]);
	}

	UE_LOG(LogTestGymsReplicationGraph, Display, TEXT("Setting Frequency Buckets to %d"), Buckets);
	for (TObjectIterator<UReplicationGraphNode_ActorListFrequencyBuckets> It; It; ++It)
	{
		UReplicationGraphNode_ActorListFrequencyBuckets* Node = *It;
		Node->SetNonStreamingCollectionSize(Buckets);
	}
}));
