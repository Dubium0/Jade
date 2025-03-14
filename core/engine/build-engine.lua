project "Engine"
   kind "SharedLib"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"
   staticruntime "off"
    
   files {  "headers/**.hpp",
            "headers/**.h", 
            "sources/**.cpp",
            -- some dependencies will be built with engine project
            VENDOR_ROOT .. "/imgui/*.cpp",
            VENDOR_ROOT .. "/imgui/backends/imgui_impl_vulkan.cpp",
            VENDOR_ROOT .. "/imgui/backends/imgui_impl_vulkan.h",
            VENDOR_ROOT .. "/imgui/backends/imgui_impl_sdl2.cpp",
            VENDOR_ROOT .. "/imgui/backends/imgui_impl_sdl2.h",
            VENDOR_ROOT .. "/vk-bootstrap/src/VkBootstrap.cpp",
            VENDOR_ROOT .. "/vk-bootstrap/src/VkBootstrap.h",
            VENDOR_ROOT .. "/vk-bootstrap/src/VkBootstrapDispatch.h"
        }

   dependson "ShaderCompiler"

   includedirs{
        "headers",

     -- dependencies
        "$(VULKAN_SDK)/Include",
        VENDOR_ROOT .. "/fastgltf/include",
        VENDOR_ROOT .. "/fmt/include",
        VENDOR_ROOT .. "/glm",
        VENDOR_ROOT .. "/SDL2/include",
        VENDOR_ROOT .. "/stb",
        VENDOR_ROOT .. "/vk-bootstrap/src",
        VENDOR_ROOT .. "/VulkanMemoryAllocator/include",
        VENDOR_ROOT .. "/imgui"
   }

 
   targetdir ("../../bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../../bin/int/" .. OutputDir .. "/%{prj.name}")
   assetsPath = ROOT_JOIN "assets"
   
   filter "system:windows"
       systemversion "latest"
       defines { 'ASSETS_PATH="' .. assetsPath .. '"', 'SHADERS_PATH="' .. shaderOutputDir .. '"'}

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
       libdirs {
        "$(VULKAN_SDK)/Lib",
        VENDOR_ROOT .. "/fastgltf/build/%{cfg.buildcfg}",
        VENDOR_ROOT .. "/fmt/build/%{cfg.buildcfg}",
        VENDOR_ROOT .. "/SDL2/build/%{cfg.buildcfg}"
        }
    
       links { 
        "vulkan-1",
        "SDL2d",
        "fastgltf",
        "fmtd",
        }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       libdirs {
        "$(VULKAN_SDK)/Lib",
        VENDOR_ROOT .. "/fastgltf/build/%{cfg.buildcfg}",
        VENDOR_ROOT .. "/fmt/build/%{cfg.buildcfg}",
        VENDOR_ROOT .. "/SDL2/build/%{cfg.buildcfg}"
        }
    
       links { 
        "vulkan-1",
        "SDL2",
        "fastgltf",
        "fmt",
        }

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"
       libdirs {
        "$(VULKAN_SDK)/Lib",
        VENDOR_ROOT .. "/fastgltf/build/Release",
        VENDOR_ROOT .. "/fmt/build/Release",
        VENDOR_ROOT .. "/SDL2/build/Release"
        }
        links { 
            "vulkan-1",
            "SDL2",
            "fastgltf",
            "fmt",
        }