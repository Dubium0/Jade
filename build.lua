-- premake5.lua
workspace "Jade"
   architecture "x64"
   configurations { "Debug", "Release", "Dist"}
   startproject "app"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus","/utf-8" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

ROOT_JOIN = function (path)
   return string.format("%s/%s", _MAIN_SCRIPT_DIR, path)
end
print(_MAIN_SCRIPT_DIR)

VENDOR_ROOT = ROOT_JOIN "vendor"
ENGINE_ROOT = ROOT_JOIN "core/engine"
ECS_ROOT = ROOT_JOIN "core/ecs"
APP_ROOT = ROOT_JOIN "app"

group "Core"
	include "core/build-core.lua"
group ""
   include "app/build-app.lua"