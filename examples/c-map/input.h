// Decodes host input and submits camera commands directly.

#ifndef C_MAP_INPUT_H
#define C_MAP_INPUT_H

#include <SDL3/SDL.h>

#include "map_state.h"
#include "types.h"

typedef enum drag_mode : uint8_t {
  DRAG_MODE_NONE,
  DRAG_MODE_PAN,
  DRAG_MODE_ROTATE,
} drag_mode;

typedef struct input_controller {
  drag_mode drag_mode;
  uint8_t drag_button;
  double last_x;
  double last_y;
} input_controller;

/// Submits the camera commands one input event makes. The map reports the
/// resulting render update, which drives the next frame demand.
[[nodiscard]] app_error input_controller_handle_event(
  input_controller* controller, const SDL_Event* event, map_state* state,
  viewport current_viewport
);

void input_log_controls(void);

#endif  // C_MAP_INPUT_H
