// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class GDKTestGymsClientTarget : TargetRules
{
	public GDKTestGymsClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		ExtraModuleNames.Add("GDKTestGyms");

        bUseChecksInShipping = bUseLoggingInShipping = true;
    }
}
