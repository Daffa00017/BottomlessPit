// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BottomlessPit : ModuleRules
{
	public BottomlessPit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"UMG", 
			"Slate",
			"SlateCore",
			"PaperZD",
			"Paper2D",
            "GameplayCameras"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

	}
}



