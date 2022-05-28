#include "shaders.hpp"

#include "headers/shader.vs.hpp"
#include "headers/shader.ps.hpp"
#include "headers/gbuffer_read.vs.hpp"
#include "headers/gbuffer_read.ps.hpp"
#include "headers/gbuffer_write.vs.hpp"
#include "headers/gbuffer_write.ps.hpp"

const size_t shader_vs_size = sizeof(shader_vs);
const size_t shader_ps_size = sizeof(shader_ps);

const size_t gbuffer_read_vs_size = sizeof(gbuffer_read_vs);
const size_t gbuffer_read_ps_size = sizeof(gbuffer_read_ps);
const size_t gbuffer_write_vs_size = sizeof(gbuffer_write_vs);
const size_t gbuffer_write_ps_size = sizeof(gbuffer_write_ps);