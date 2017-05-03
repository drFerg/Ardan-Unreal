// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Ardan : ModuleRules
{
	public Ardan(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "Sockets",
		"Networking" });

        PublicIncludePaths.AddRange(new string[] { "C:/Users/fwl14/Documents/GitHub/flatbuffers/include/" });
    }
}
