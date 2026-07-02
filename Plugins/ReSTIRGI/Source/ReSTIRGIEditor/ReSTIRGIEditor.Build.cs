using UnrealBuildTool;

public class ReSTIRGIEditor : ModuleRules
{
	public ReSTIRGIEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ReSTIRGIRuntime",
			"Slate",
			"SlateCore",
			"ToolMenus"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"LevelEditor",
			"Projects",
			"UnrealEd"
		});
	}
}
