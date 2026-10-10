using UnrealBuildTool;

public class FuserServer : ModuleRules
{
    public FuserServer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Hype", "OnlineSubsystemUtils" });
        PrivateDependencyModuleNames.AddRange(new[] { "PakFile", "EngineSettings", "HTTP", "Json" }); // FDSInstallContent, FDSVenues, FDSRegistration
    }
}
