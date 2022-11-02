#pragma once

struct PC
{
	uint8_t iRenderMode = 0;
	uint8_t iPadding0 = 0; // not used yet
	uint8_t iPadding1 = 0; // not used yet
	uint8_t iPadding2 = 0; // not used yet
	float depthModA = 0.0f;
	float depthModB = 1.0f;
	bool bGradientFillers = true;
};