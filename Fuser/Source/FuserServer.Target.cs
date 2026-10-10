using UnrealBuildTool;

public class FuserServerTarget : TargetRules
{
    public FuserServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange(new[] { "FuserServer", "Hype", "OnlineSubsystemHype" });
    }
}
