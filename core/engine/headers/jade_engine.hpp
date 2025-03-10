#pragma once

#include <jade_structs.hpp>

#define ENGINE_API_EXPORT __declspec(dllexport)
#define ENGINE_API_IMPORT __declspec(dllimport)
extern "C" {
   
    ENGINE_API_EXPORT void sayHelloFromEngine();
    ENGINE_API_EXPORT const char* getEngineMessage();

    namespace jade{

        ENGINE_API_EXPORT void initEngine(EngineCreateInfo createInfo);
        ENGINE_API_EXPORT void runEngine();
        ENGINE_API_EXPORT void cleanupEngine();

    };
}