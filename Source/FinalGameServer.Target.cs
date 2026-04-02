using UnrealBuildTool;
using System.Collections.Generic;

public class FinalGameServerTarget : TargetRules
{
    public FinalGameServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server; 
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("FinalGame"); 
    }
}