using UnrealBuildTool;

public class FuserServerServerTarget : TargetRules
{
    public FuserServerServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange(new[] { "FuserServer", "Hype", "OnlineSubsystemHype" });
    }
}
