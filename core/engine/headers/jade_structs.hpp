#pragma once

#include <string>
#include <stdint.h>

#define ENGINE_API_EXPORT __declspec(dllexport)
#define ENGINE_API_IMPORT __declspec(dllimport)

namespace jade{
	extern "C" {
		struct ENGINE_API_EXPORT EngineCreateInfo {

			const char* windowTitle;
			uint32_t windowWidth;
			uint32_t windowHeight;

		};
	}
}