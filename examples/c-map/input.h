// The input module: decodes host input into camera commands.
//
// This runs on the render loop, which does not own the map, so it only
// produces commands; the runtime loop applies them on the map's owner thread.
// Anything needing the current viewport is converted to logical map
// coordinates here, where the viewport lives.

#ifndef C_MAP_INPUT_H
#define C_MAP_INPUT_H

#include <SDL3/SDL.h>

#include "channel.h"
#include "types.h"

typedef enum drag_mode : uint8_t {
  DRAG_MODE_NONE,
  DRAG_MODE_PAN,
  DRAG_MODE_ROTATE,
} drag_mode;

typedef struct input_result {
  bool handled;
  bool camera_changed;
} input_result;

typedef struct input_controller {
  drag_mode drag_mode;
  /// The button that started the live drag. A drag belongs to one button, so
  /// a second button pressed during it neither restarts it nor ends it early.
  uint8_t drag_button;
  double last_x;
  double last_y;
} input_controller;

input_result input_controller_handle_event(
  input_controller* controller, const SDL_Event* event, command_queue* commands,
  viewport current_viewport
);

void input_log_controls(void);

#endif  // C_MAP_INPUT_H
