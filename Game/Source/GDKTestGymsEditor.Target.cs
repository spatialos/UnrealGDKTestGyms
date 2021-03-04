// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class GDKTestGymsEditorTarget : TargetRules
{
	public GDKTestGymsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		ExtraModuleNames.Add("GDKTestGyms");
		ExtraModuleNames.Add("GDKTestGymsFunctionalTests");
	}
}
