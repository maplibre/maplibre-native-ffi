// The viewport module: reads the window's logical size, physical size, and
// content scale from SDL, and derives the logical map extent from them.

#ifndef C_MAP_VIEWPORT_H
#define C_MAP_VIEWPORT_H

#include <SDL3/SDL.h>

#include "types.h"

static constexpr int viewport_window_width = 960;
static constexpr int viewport_window_height = 640;

viewport viewport_get(SDL_Window* window);
void viewport_log(const char* label, viewport value);

#endif  // C_MAP_VIEWPORT_H
