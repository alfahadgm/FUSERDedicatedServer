using UnrealBuildTool;

public class FuserServerEditorTarget : TargetRules
{
    public FuserServerEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange(new[] { "FuserServer", "Hype", "OnlineSubsystemHype", "FDSEditor" });
    }
}
