#pragma once

#include <chrono>
#include <iostream>
#include <vector>
#include <queue>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

// Enable the WSI extensions
#if defined(_WIN32)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif

// disable warnings for external libraries
#pragma warning(push, 0)
	// include the hpp wrapper version of vulkan
	// use dynamic dispatcher
	#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
	#include <vulkan/vulkan.hpp>

	// Vulkan Memory Allocator with hpp bindings
	// -> stb-style lib included and implemented in pch.cpp

	#define GLM_FORCE_RADIANS
	#define GLM_FORCE_DEPTH_ZERO_TO_ONE
	#define GLM_FORCE_LEFT_HANDED
	#define GLM_FORCE_SIZE_T_LENGTH
	#include <glm/glm.hpp>
	#include <glm/gtc/matrix_transform.hpp>
	#include <glm/gtc/quaternion.hpp>

	// Tell SDL not to mess with main()
	#define SDL_MAIN_HANDLED
	#include <SDL.h>
	#include <SDL_syswm.h>
	#include <SDL_vulkan.h>

	// 3rd party libs
	#include "entt.hpp"
	#include "imgui.h"
	#include "backends/imgui_impl_sdl.h"
	#include "backends/imgui_impl_vulkan.h"
#pragma warning(pop)

// Utils
#include "utils/logging.hpp"
#include "utils/timer.hpp"
#include "utils/rule_of_five.hpp"



// undefine some whacky stuff that are defined in one of these headers for some reason
#undef near
#undef far
