// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

namespace UnrealBuildTool.Rules
{
	public class MetricsServiceProviderTests : ModuleRules
	{
		public MetricsServiceProviderTests(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"Http",
					"Json"
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Analytics",
					"MetricsServiceProvider"
				}
				);

			PublicIncludePathModuleNames.Add("Analytics");
		}
	}
}
