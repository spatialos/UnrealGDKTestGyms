// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;
using System; 
using System.Collections.Generic;

public class GDKTestGymsTarget : TargetRules
{
	public GDKTestGymsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		ExtraModuleNames.Add("GDKTestGyms");

		if (Environment.GetEnvironmentVariable("ImprobableNFRShipping") != null)
		{
			//bUseLoggingInShipping = bEnableSpatialCmdlineInShipping = true;
		}

		if (Environment.GetEnvironmentVariable("ImprobableNFRStats") != null)
		{
			GlobalDefinitions.Add("FORCE_USE_STATS=1");
		}
	}
}
