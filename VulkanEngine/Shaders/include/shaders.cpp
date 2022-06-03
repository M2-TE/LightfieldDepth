#include "shaders.hpp"

#include "headers/lighting_pass.vs.hpp"
#include "headers/lighting_pass.ps.hpp"
#include "headers/geometry_pass.vs.hpp"
#include "headers/geometry_pass.ps.hpp"

const size_t lighting_pass_vs_size = sizeof(lighting_pass_vs);
const size_t lighting_pass_ps_size = sizeof(lighting_pass_ps);
const size_t geometry_pass_vs_size = sizeof(geometry_pass_vs);
const size_t geometry_pass_ps_size = sizeof(geometry_pass_ps);