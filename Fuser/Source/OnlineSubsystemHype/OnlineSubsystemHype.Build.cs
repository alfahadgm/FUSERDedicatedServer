using UnrealBuildTool;

// /Script/OnlineSubsystemHype: the FUSER types that live in that script package (EHypeOnlinePlatform), generated.
// Not an online subsystem implementation.
public class OnlineSubsystemHype : ModuleRules
{
    public OnlineSubsystemHype(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject" });
    }
}
