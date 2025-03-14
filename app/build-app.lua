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
	  "../core/engine/headers",
      "../core/ecs/headers",

      VENDOR_ROOT .. "/fmt/include"
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

       libdirs {
        VENDOR_ROOT .. "/fmt/build/%{cfg.buildcfg}"
        }
       links { 
        "fmtd"
        }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       postbuildcommands {
        "{COPY} %{wks.location}vendor/SDL2/build/%{cfg.buildcfg}/*.dll %{cfg.targetdir}"
       }

       libdirs {
        VENDOR_ROOT .. "/fmt/build/%{cfg.buildcfg}"
        }
       links { 
        "fmt"
        }
   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"
       postbuildcommands {
        "{COPY} %{wks.location}vendor/SDL2/build/Release/*.dll %{cfg.targetdir}"
       }

       libdirs {
        VENDOR_ROOT .. "/fmt/build/%{cfg.buildcfg}"
        }
       links { 
        "fmt"
        }