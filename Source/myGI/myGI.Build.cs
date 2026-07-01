// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class myGI : ModuleRules
{
	public myGI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"myGI",
			"myGI/Variant_Platforming",
			"myGI/Variant_Platforming/Animation",
			"myGI/Variant_Combat",
			"myGI/Variant_Combat/AI",
			"myGI/Variant_Combat/Animation",
			"myGI/Variant_Combat/Gameplay",
			"myGI/Variant_Combat/Interfaces",
			"myGI/Variant_Combat/UI",
			"myGI/Variant_SideScrolling",
			"myGI/Variant_SideScrolling/AI",
			"myGI/Variant_SideScrolling/Gameplay",
			"myGI/Variant_SideScrolling/Interfaces",
			"myGI/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
