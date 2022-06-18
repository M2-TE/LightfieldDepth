#pragma once

struct ShaderData { const unsigned char* pData; size_t size; };
struct ShaderPack { ShaderData vs, ps; };

extern const ShaderPack geometryPass, lightingPass;
extern const ShaderPack swapchainWrite;
