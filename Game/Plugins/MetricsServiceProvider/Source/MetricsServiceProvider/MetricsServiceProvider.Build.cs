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
#if UE_5_0_OR_NEWER
					"HTTP",
#else
					"Http",
#endif
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
