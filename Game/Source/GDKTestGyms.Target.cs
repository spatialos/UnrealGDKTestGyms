// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class GDKTestGymsTarget : TargetRules
{
	public GDKTestGymsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		ExtraModuleNames.Add("GDKTestGyms");

		bUseLoggingInShipping = bEnableSpatialCmdlineInShipping = true;
	}
}
