#include <math.h>
#include <stdio.h>

#include "input.h"

static constexpr double keyboard_animation_ms = 160.0;
static constexpr double reset_animation_ms = 220.0;

static double logical_coordinate(
  double value, uint32_t window_size, uint32_t logical_size
) {
  if (window_size == 0) {
    return value;
  }
  return value * (double)logical_size / (double)window_size;
}

static mln_screen_point logical_point(
  double x, double y, viewport current_viewport
) {
  return (mln_screen_point){
    .x = logical_coordinate(
      x, current_viewport.window_width, current_viewport.logical_width
    ),
    .y = logical_coordinate(
      y, current_viewport.window_height, current_viewport.logical_height
    ),
  };
}

static camera_command pan_animated(double dx, double dy) {
  return (camera_command){
    .kind = CAMERA_COMMAND_MOVE_BY_ANIMATED,
    .as.move_by_animated = {
      .dx = dx, .dy = dy, .duration_ms = keyboard_animation_ms
    },
  };
}

static camera_command zoom_animated(double scale, mln_screen_point anchor) {
  return (camera_command){
    .kind = CAMERA_COMMAND_SCALE_BY_ANIMATED,
    .as.scale_by_animated = {
      .scale = scale, .anchor = anchor, .duration_ms = keyboard_animation_ms
    },
  };
}

static camera_command rotate_animated(double delta) {
  return (camera_command){
    .kind = CAMERA_COMMAND_ADJUST_BEARING_ANIMATED,
    .as.animated_delta = {.delta = delta, .duration_ms = keyboard_animation_ms},
  };
}

static camera_command pitch_animated(double delta) {
  return (camera_command){
    .kind = CAMERA_COMMAND_ADJUST_PITCH_ANIMATED,
    .as.animated_delta = {.delta = delta, .duration_ms = keyboard_animation_ms},
  };
}

static drag_mode drag_mode_for_button(uint8_t button) {
  if (button == SDL_BUTTON_RIGHT) {
    return DRAG_MODE_ROTATE;
  }
  if (button != SDL_BUTTON_LEFT) {
    return DRAG_MODE_NONE;
  }
  if ((SDL_GetModState() & SDL_KMOD_CTRL) != 0) {
    return DRAG_MODE_ROTATE;
  }
  return DRAG_MODE_PAN;
}

static input_result handle_mouse_button_down(
  input_controller* controller, const SDL_MouseButtonEvent* button,
  command_queue* commands, viewport current_viewport
) {
  // A second button joins the live drag rather than starting one, leaving the
  // drag's baseline position alone.
  if (controller->drag_mode != DRAG_MODE_NONE) {
    return (input_result){.handled = true};
  }

  const drag_mode mode = drag_mode_for_button(button->button);
  if (mode == DRAG_MODE_NONE) {
    return (input_result){};
  }

  const mln_screen_point cursor =
    logical_point(button->x, button->y, current_viewport);
  controller->last_x = cursor.x;
  controller->last_y = cursor.y;

  // Queued first, so any transition stops before the first delta lands.
  command_queue_push(
    commands, (camera_command){.kind = CAMERA_COMMAND_CANCEL_TRANSITIONS}
  );
  command_queue_push(
    commands, (camera_command){
                .kind = CAMERA_COMMAND_SET_GESTURE_IN_PROGRESS,
                .as.set_gesture_in_progress = {.in_progress = true},
              }
  );
  controller->drag_mode = mode;
  controller->drag_button = button->button;
  return (input_result){.handled = true};
}

/// Every path that ends a drag runs through here, so the gesture bracket the
/// drag opened is always closed.
static void end_drag(input_controller* controller, command_queue* commands) {
  if (controller->drag_mode == DRAG_MODE_NONE) {
    return;
  }
  controller->drag_mode = DRAG_MODE_NONE;
  controller->drag_button = 0;
  command_queue_push(
    commands, (camera_command){
                .kind = CAMERA_COMMAND_SET_GESTURE_IN_PROGRESS,
                .as.set_gesture_in_progress = {.in_progress = false},
              }
  );
}

static input_result handle_mouse_button_up(
  input_controller* controller, const SDL_MouseButtonEvent* button,
  command_queue* commands
) {
  if (button->button != SDL_BUTTON_LEFT && button->button != SDL_BUTTON_RIGHT) {
    return (input_result){};
  }
  // The drag ends once, when the button that started it comes up.
  if (button->button != controller->drag_button) {
    return (input_result){.handled = true};
  }
  end_drag(controller, commands);
  controller->last_x = button->x;
  controller->last_y = button->y;
  return (input_result){.handled = true};
}

static input_result handle_mouse_motion(
  input_controller* controller, const SDL_MouseMotionEvent* motion,
  command_queue* commands, viewport current_viewport
) {
  if (controller->drag_mode == DRAG_MODE_NONE) {
    return (input_result){};
  }

  const mln_screen_point cursor =
    logical_point(motion->x, motion->y, current_viewport);
  const double dx = cursor.x - controller->last_x;
  const double dy = cursor.y - controller->last_y;
  controller->last_x = cursor.x;
  controller->last_y = cursor.y;
  if (dx == 0 && dy == 0) {
    return (input_result){.handled = true};
  }

  switch (controller->drag_mode) {
    case DRAG_MODE_NONE:
      break;
    case DRAG_MODE_PAN:
      command_queue_push(
        commands, (camera_command){
                    .kind = CAMERA_COMMAND_MOVE_BY,
                    .as.move_by = {.dx = dx, .dy = dy},
                  }
      );
      break;
    case DRAG_MODE_ROTATE:
      command_queue_push(
        commands, (camera_command){
                    .kind = CAMERA_COMMAND_ADJUST_BEARING,
                    .as.delta = {.delta = dx * 0.5},
                  }
      );
      command_queue_push(
        commands, (camera_command){
                    .kind = CAMERA_COMMAND_PITCH_BY,
                    .as.delta = {.delta = dy / 2.0},
                  }
      );
      break;
  }
  return (input_result){.handled = true, .camera_changed = true};
}

static input_result handle_mouse_wheel(
  const SDL_MouseWheelEvent* wheel, command_queue* commands,
  viewport current_viewport
) {
  const double delta = wheel->y;
  if (delta == 0) {
    return (input_result){.handled = true};
  }

  const mln_screen_point anchor =
    logical_point(wheel->mouse_x, wheel->mouse_y, current_viewport);
  command_queue_push(
    commands,
    (camera_command){
      .kind = CAMERA_COMMAND_SCALE_BY,
      .as.scale_by = {.scale = pow(2.0, delta * 0.25), .anchor = anchor},
    }
  );
  return (input_result){.handled = true, .camera_changed = true};
}

static input_result handle_key_down(
  const SDL_KeyboardEvent* key, command_queue* commands,
  viewport current_viewport
) {
  constexpr double pan_step = 120.0;
  constexpr double zoom_step = 1.25;
  constexpr double bearing_step = 10.0;
  constexpr double pitch_step = 5.0;
  const mln_screen_point center = {
    .x = (double)current_viewport.logical_width / 2.0,
    .y = (double)current_viewport.logical_height / 2.0,
  };

  camera_command command;
  switch (key->scancode) {
    case SDL_SCANCODE_LEFT:
    case SDL_SCANCODE_A:
      command = pan_animated(pan_step, 0);
      break;
    case SDL_SCANCODE_RIGHT:
    case SDL_SCANCODE_D:
      command = pan_animated(-pan_step, 0);
      break;
    case SDL_SCANCODE_UP:
    case SDL_SCANCODE_W:
      command = pan_animated(0, pan_step);
      break;
    case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_S:
      command = pan_animated(0, -pan_step);
      break;
    case SDL_SCANCODE_EQUALS:
    case SDL_SCANCODE_KP_PLUS:
      command = zoom_animated(zoom_step, center);
      break;
    case SDL_SCANCODE_MINUS:
    case SDL_SCANCODE_KP_MINUS:
      command = zoom_animated(1.0 / zoom_step, center);
      break;
    case SDL_SCANCODE_Q:
      command = rotate_animated(-bearing_step);
      break;
    case SDL_SCANCODE_E:
      command = rotate_animated(bearing_step);
      break;
    case SDL_SCANCODE_RIGHTBRACKET:
      command = pitch_animated(pitch_step);
      break;
    case SDL_SCANCODE_LEFTBRACKET:
      command = pitch_animated(-pitch_step);
      break;
    case SDL_SCANCODE_0:
      command = (camera_command){
        .kind = CAMERA_COMMAND_RESET_ORIENTATION,
        .as.reset_orientation = {.duration_ms = reset_animation_ms},
      };
      break;
    default:
      return (input_result){};
  }

  command_queue_push(commands, command);
  return (input_result){.handled = true, .camera_changed = true};
}

input_result input_controller_handle_event(
  input_controller* controller, const SDL_Event* event, command_queue* commands,
  viewport current_viewport
) {
  switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      return handle_mouse_button_down(
        controller, &event->button, commands, current_viewport
      );
    case SDL_EVENT_MOUSE_BUTTON_UP:
      return handle_mouse_button_up(controller, &event->button, commands);
    case SDL_EVENT_MOUSE_MOTION:
      return handle_mouse_motion(
        controller, &event->motion, commands, current_viewport
      );
    case SDL_EVENT_MOUSE_WHEEL:
      return handle_mouse_wheel(&event->wheel, commands, current_viewport);
    case SDL_EVENT_KEY_DOWN:
      return handle_key_down(&event->key, commands, current_viewport);
    default:
      return (input_result){};
  }
}

void input_log_controls(void) {
  printf(
    "Controls:\n"
    "  left drag: pan\n"
    "  right drag or Ctrl+left drag: rotate with X, pitch with Y\n"
    "  scroll: zoom at cursor\n"
    "  arrows or WASD: pan\n"
    "  + / -: zoom at center\n"
    "  Q / E: rotate\n"
    "  ] / [: pitch\n"
    "  0: reset pitch and bearing\n"
  );
}
