// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;

public class GDKTestGyms : ModuleRules
{
	public GDKTestGyms(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"GameplayAbilities",
				"GameplayTasks",
				"InputCore",
				"Sockets",
				"OnlineSubsystemUtils",
				"PhysXVehicles",
				"SpatialGDK",
				"ReplicationGraph",
				"AIModule",
				"NavigationSystem"
			});
	}
}
