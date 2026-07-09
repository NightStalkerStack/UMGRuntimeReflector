using UnrealBuildTool;

public class UMGRuntimeReflector : ModuleRules
{
	public UMGRuntimeReflector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EditorStyle",
			"LevelEditor",
			"PropertyEditor",
			"ToolMenus",
			"UnrealEd",
			"WorkspaceMenuStructure"
		});
	}
}
