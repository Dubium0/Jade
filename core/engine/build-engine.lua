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
            "../vendor/imgui/**.cpp",
            "../vendor/imgui/backends/imgui_impl_vulkan.cpp",
            "../vendor/imgui/backends/imgui_impl_sdl2.cpp",
            "../vendor/vk-bootstrap/src/**.cpp"
        }

   includedirs
   {
     "headers",

     -- dependencies
     "../vendor/fastgltf/include",
     "../vendor/fmt/include",
     "../vendor/glm/glm",
     "../vendor/SDL2/include",
     "../vendor/stb",
     "../vendor/vk-bootstrap/src",
     "../vendor/VulkanMemoryAllocator/include",
     "../vendor/imgui"
   }

   libdirs {
        "../vendor/fastgltf/build/Release",
        "../vendor/fmt/build/Release",
        "../vendor/SDL2/build/Release"
   }

   links { 
        "SDL2",
        "fastgltf",
        "fmt"
   }
   targetdir ("../bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../bin/int/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"