// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Virtual_CPS_World : ModuleRules
{
	public Virtual_CPS_World(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", 
															"CoreUObject", 
															"Engine", 
															"InputCore",
															"Sockets",
															"Networking" });
	}
}
