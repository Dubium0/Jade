project "ShaderCompiler"
   kind "Utility" -- A utility project doesn't produce an executable or library

   targetdir ("../../bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../../bin/int/" .. OutputDir .. "/%{prj.name}")

   -- Define shader files
   files { "**.vert", "**.frag", "**.comp", "**.glsl" }
   SHADER_ABS = ROOT_JOIN "core/shaders"
   shaderOutputDir = ROOT_JOIN "bin/" .. OutputDir .. "/ShaderCompiler"
   
   --prebuildcommands { "{MKDIR} " .. shaderOutputDir }
   filter "files:*.vert"  
        buildmessage "Compiling %{SHADER_ABS}/%{file.name} into SPIR-V..." --    "Compiling %[shaders\\%{file.name}] into SPIR-V..."
        buildcommands { 
            '"$(VULKAN_SDK)\\Bin\\glslangValidator.exe" -V -o "%{shaderOutputDir}/%{file.name}.spv" "%[%{SHADER_ABS}/%{file.name}]"'
        }
        buildoutputs { '%{shaderOutputDir}/%{file.name}.spv' }

   filter "files:*.frag"  
        buildmessage "Compiling %{SHADER_ABS}/%{file.name} into SPIR-V..." --    "Compiling %[shaders\\%{file.name}] into SPIR-V..."
        buildcommands { 
            '"$(VULKAN_SDK)\\Bin\\glslangValidator.exe" -V -o "%{shaderOutputDir}/%{file.name}.spv" "%[%{SHADER_ABS}/%{file.name}]"'
        }
        buildoutputs { '%{shaderOutputDir}/%{file.name}.spv' }

   filter "files:*.comp"  
        buildmessage "Compiling %{SHADER_ABS}/%{file.name} into SPIR-V..." --    "Compiling %[shaders\\%{file.name}] into SPIR-V..."
        buildcommands { 
            '"$(VULKAN_SDK)\\Bin\\glslangValidator.exe" -V -o "%{shaderOutputDir}/%{file.name}.spv" "%[%{SHADER_ABS}/%{file.name}]"'
        }
        buildoutputs { '%{shaderOutputDir}/%{file.name}.spv' }