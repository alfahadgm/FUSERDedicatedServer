using UnrealBuildTool;

// /Script/Hype: FUSER's network-visible types, generated (see generated-files.txt) plus
// hand-written serializers and behaviour in Private/.
public class Hype : ModuleRules
{
    public Hype(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // FUSER's RPC parameter names (e.g. Tick, Key) shadow engine members; the names are kept as recorded.
        ShadowVariableWarningLevel = WarningLevel.Warning;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "OnlineSubsystemHype" });
        // HypeMultiplayerBeacon.h: the session-browser beacon derives from the stock online beacons
        PublicDependencyModuleNames.AddRange(new[] { "OnlineSubsystem", "OnlineSubsystemUtils" });
    }
}
