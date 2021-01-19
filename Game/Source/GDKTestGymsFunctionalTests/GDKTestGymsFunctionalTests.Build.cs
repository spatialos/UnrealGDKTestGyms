using UnrealBuildTool;

public class GDKTestGymsFunctionalTests : ModuleRules
{
    public GDKTestGymsFunctionalTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
			"SpatialGDKFunctionalTests"
        });
    }
}
