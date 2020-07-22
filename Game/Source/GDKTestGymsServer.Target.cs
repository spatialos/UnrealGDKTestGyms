// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class GDKTestGymsServerTarget : TargetRules
{
	public GDKTestGymsServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		ExtraModuleNames.Add("GDKTestGyms");

        bUseLoggingInShipping = bEnableSpatialCmdlineInShipping = true;
    }
}
