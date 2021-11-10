// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

namespace UnrealBuildTool.Rules
{
	public class MetricsServiceProvider : ModuleRules
	{
		public MetricsServiceProvider(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivatePCHHeaderFile = "Public/MetricsServiceModule.h";
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"HTTP",
					"HTTPServer",
					"Json",
					"JsonUtilities"
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Analytics"
				}
				);

			PublicIncludePathModuleNames.Add("Analytics");
		}
	}
}
