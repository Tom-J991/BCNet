#pragma once

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define SIMULATION_TIMESTEP (1.0f / 60.0f)

namespace GLOBALS
{
	// Shouldn't really use globals like this but it makes it a lot easier.
	inline bool g_DEBUG = false; // Debug view flag.

	inline aie::Font *g_font; // Global reference to font texture.
	
}
