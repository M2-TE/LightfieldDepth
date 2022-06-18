#include "shaders.hpp"

#include "headers/lighting_pass.vs.hpp"
#include "headers/lighting_pass.ps.hpp"
#include "headers/geometry_pass.vs.hpp"
#include "headers/geometry_pass.ps.hpp"
#include "headers/swapchain_write.vs.hpp"
#include "headers/swapchain_write.ps.hpp"

const ShaderPack geometryPass = { { geometry_pass_vs, sizeof(geometry_pass_vs) }, { geometry_pass_ps, sizeof(geometry_pass_ps) } };
const ShaderPack lightingPass = { { lighting_pass_vs, sizeof(lighting_pass_vs) }, { lighting_pass_ps, sizeof(lighting_pass_ps) } };
const ShaderPack swapchainWrite = { { swapchain_write_vs, sizeof(swapchain_write_vs) }, { swapchain_write_ps, sizeof(swapchain_write_ps) } };