using UnrealBuildTool;

// Editor-only FDS tools (asset creation commandlets). Not part of the server or game targets.
public class FDSEditor : ModuleRules
{
    public FDSEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "AssetRegistry" });
    }
}
