// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class FinalGame : ModuleRules
{
    public FinalGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            // Core Engine Modules
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            
            // Stable OSSv1 Modules (EOS and Base)
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "OnlineSubsystemEOS", 
            
            // Networking and Data Modules
            "HTTP",
            "WebSockets",
            "Json",
            "JsonUtilities"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}