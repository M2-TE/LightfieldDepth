#pragma once

#include <chrono>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

// Enable the WSI extensions
#if defined(_WIN32)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif

// include the hpp wrapper of vulkan
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_SIZE_T_LENGTH
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>


// 3rd party libs
#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_vulkan.h"

// Utils
#include "utils/logging.hpp"
#include "utils/timer.hpp"
#include "utils/rule_of_five.hpp"
