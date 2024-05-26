// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

using UnrealBuildTool;

public class SaveGamePluginNodes : ModuleRules
{
    public SaveGamePluginNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "SaveGamePlugin",
                "CoreUObject",
                "Engine",
                "SlateCore",
                "ToolMenus",
                "BlueprintGraph",
                "UnrealEd",
                "KismetCompiler",
            }
        );
    }
}
