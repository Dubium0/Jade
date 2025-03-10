project "App"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir ("../bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../bin/int/" .. OutputDir .. "/%{prj.name}")
   staticruntime "off"

   files { "headers/**.hpp","headers/**.h", "sources/**.cpp" }

   includedirs
   {
      "headers",

	  -- Include Core
	  "../core/engine/headers"
   }

   links
   {
      "Engine"
   }
  
   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }
       postbuildcommands {
        "{COPY} %{wks.location}bin/" .. OutputDir .. "/Engine/*.dll %{cfg.targetdir}"
       }
       --assuming SDL2 binaries are inside the build directory ( default behavior)
      

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
       postbuildcommands {
        "{COPY} %{wks.location}vendor/SDL2/build/%{cfg.buildcfg}/*.dll %{cfg.targetdir}"
       }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       postbuildcommands {
        "{COPY} %{wks.location}vendor/SDL2/build/%{cfg.buildcfg}/*.dll %{cfg.targetdir}"
       }
   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"
       postbuildcommands {
        "{COPY} %{wks.location}vendor/SDL2/build/Release/*.dll %{cfg.targetdir}"
       }