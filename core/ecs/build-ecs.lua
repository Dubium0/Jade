project "ECS"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"
   staticruntime "off"
    
  
   files {  "headers/**.hpp",
            "headers/**.h", 
            "sources/**.cpp"
        }



   includedirs{
        "headers",
        --vendors
        VENDOR_ROOT .. "/fmt/include"
   }

 
   targetdir ("../../bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../../bin/int/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
      

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
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
       libdirs {
        VENDOR_ROOT .. "/fmt/build/%{cfg.buildcfg}"
        }
       links { 
        "fmt"
        }
       