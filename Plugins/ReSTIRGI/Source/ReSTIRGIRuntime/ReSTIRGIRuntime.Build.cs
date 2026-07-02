using UnrealBuildTool;

public class ReSTIRGIRuntime : ModuleRules
{
	public ReSTIRGIRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"Renderer",
			"RHI",
			"Projects"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"ApplicationCore"
		});

		PrivateIncludePaths.AddRange(new[]
		{
			System.IO.Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
			System.IO.Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal")
		});
	}
}
