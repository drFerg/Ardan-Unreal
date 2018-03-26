// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Ardan : ModuleRules
{
	public Ardan(TargetInfo Target)
	{

        //string home = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../"));
        //cout << home;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "Sockets",
		"Networking" });

        PublicIncludePaths.AddRange(new string[] { "C:/Users/fwl14/Documents/GitHub/flatbuffers/include/" });
        //PublicIncludePaths.Add("C:/Users/Fergus/Documents/GitHub/SRPC/src/include");
        //PublicAdditionalLibraries.Add("C:/Users/Fergus/Documents/GitHub/SRPC/libsrpc.lib");
        //PublicAdditionalLibraries.Add("C:/Program Files/cppkafka/lib/cppkafka.lib");
        //PublicIncludePaths.Add("C:/Program Files/cppkafka/include/");
        PublicAdditionalLibraries.Add("E:/Unreal/Project/Ardan 4.15 - 3/packages/librdkafka.redist.0.11.3/build/native/lib/win7/x64/win7-x64-Release/v120/librdkafka.lib");
        PublicAdditionalLibraries.Add("E:/Unreal/Project/Ardan 4.15 - 3/packages/librdkafka.redist.0.11.3/build/native/lib/win7/x64/win7-x64-Release/v120/librdkafkacpp.lib");

        PublicIncludePaths.Add("E:/Unreal/Project/Ardan 4.15 - 3/packages/librdkafka.redist.0.11.3/build/native/include");
        //PublicIncludePaths.Add("C:/Users/Fergus/Documents/Unreal Projects/Ardan/packages/boost.1.66.0.0/");
        PublicDelayLoadDLLs.Add("librdkafka.dll");
        PublicDelayLoadDLLs.Add("librdkafkacpp.dll");
        //PublicDelayLoadDLLs.Add("cppkafka.dll");
        //PublicDelayLoadDLLs.Add("libsrpc.dll");


        //PublicAdditionalLibraries.Add("C:/Users/Fergus/Documents/GitHub/SRPC/src/.libs/libpthread.a");
        //PublicAdditionalLibraries.Add("C:/Users/Fergus/Documents/GitHub/SRPC/src/.libs/libws2_32.a");
        //PublicAdditionalLibraries.Add("C:/Program Files/mingw-w64/x86_64-7.2.0-posix-seh-rt_v5-rev1/mingw64/lib/gcc/x86_64-w64-mingw32/7.2.0/libgcc.a");
    }
}
