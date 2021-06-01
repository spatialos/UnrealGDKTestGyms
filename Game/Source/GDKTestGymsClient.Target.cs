// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;
using System; 
using System.Collections.Generic;

public class GDKTestGymsClientTarget : TargetRules
{
	public GDKTestGymsClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		ExtraModuleNames.Add("GDKTestGyms");

		if (Environment.GetEnvironmentVariable("ImprobableNFRShipping") != null)
		{
			bUseLoggingInShipping = bEnableSpatialCmdlineInShipping = true;
		}

		if (Environment.GetEnvironmentVariable("ImprobableNFRStats") != null)
		{
			bUseMallocProfiler = true;
		}
	}
}
