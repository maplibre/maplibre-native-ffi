#include <math.h>
#include <stdio.h>

#include "viewport.h"

static uint32_t scaled_logical_size(int physical_size, double content_scale) {
  const double scaled = (double)physical_size / content_scale;
  const double ceiled = ceil(scaled);
  return ceiled < 1.0 ? 1u : (uint32_t)ceiled;
}

static double density(int physical_size, int logical_size) {
  return (double)physical_size / (double)logical_size;
}

viewport viewport_get(SDL_Window* window) {
  int logical_width = viewport_window_width;
  int logical_height = viewport_window_height;
  int physical_width = viewport_window_width;
  int physical_height = viewport_window_height;
  SDL_GetWindowSize(window, &logical_width, &logical_height);
  SDL_GetWindowSizeInPixels(window, &physical_width, &physical_height);

  const int safe_logical_width = logical_width > 1 ? logical_width : 1;
  const int safe_logical_height = logical_height > 1 ? logical_height : 1;
  const int safe_physical_width = physical_width > 1 ? physical_width : 1;
  const int safe_physical_height = physical_height > 1 ? physical_height : 1;
  const double density_x = density(safe_physical_width, safe_logical_width);
  const double density_y = density(safe_physical_height, safe_logical_height);
  const double size_density = density_x > density_y ? density_x : density_y;
  const float window_density = SDL_GetWindowPixelDensity(window);
  const float display_scale = SDL_GetWindowDisplayScale(window);
  const double fallback_scale =
    window_density > 0.0f ? (double)window_density : size_density;
  const double content_scale =
    display_scale > 0.0f ? (double)display_scale : fallback_scale;

  return (viewport){
    .logical_width = scaled_logical_size(safe_physical_width, content_scale),
    .logical_height = scaled_logical_size(safe_physical_height, content_scale),
    .window_width = (uint32_t)safe_logical_width,
    .window_height = (uint32_t)safe_logical_height,
    .physical_width = (uint32_t)safe_physical_width,
    .physical_height = (uint32_t)safe_physical_height,
    .scale_factor = content_scale,
  };
}

void viewport_log(const char* label, viewport value) {
  printf(
    "%s: logical=%ux%u physical=%ux%u scale=%.2f\n", label, value.logical_width,
    value.logical_height, value.physical_width, value.physical_height,
    value.scale_factor
  );
}
