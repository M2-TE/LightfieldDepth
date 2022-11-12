#pragma once

struct PC
{
	uint8_t iRenderMode = 0;
	uint8_t iPadding0 = 0; // not used yet
	uint8_t iPadding1 = 0; // not used yet
	uint8_t iPadding2 = 0; // not used yet

	// 0 = all filters combined
	// 1 = 3x3
	// 2 = 5x5
	// 3 = 7x7
	// 4 = 9x9
	uint8_t iFilterMode = 0;
	uint8_t iPadding3 = 0; // not used yet
	uint8_t iPadding4 = 0; // not used yet
	uint8_t iPadding5 = 0; // not used yet

	float depthModA = 0.0f;
	float depthModB = 1.0f;
};