// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Tinyverse : ModuleRules
{
	public Tinyverse(ReadOnlyTargetRules Target) : base(Target)
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
			"Tinyverse",
			"Tinyverse/Variant_Platforming",
			"Tinyverse/Variant_Platforming/Animation",
			"Tinyverse/Variant_Combat",
			"Tinyverse/Variant_Combat/AI",
			"Tinyverse/Variant_Combat/Animation",
			"Tinyverse/Variant_Combat/Gameplay",
			"Tinyverse/Variant_Combat/Interfaces",
			"Tinyverse/Variant_Combat/UI",
			"Tinyverse/Variant_SideScrolling",
			"Tinyverse/Variant_SideScrolling/AI",
			"Tinyverse/Variant_SideScrolling/Gameplay",
			"Tinyverse/Variant_SideScrolling/Interfaces",
			"Tinyverse/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
