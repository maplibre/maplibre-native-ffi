#include <math.h>
#include <stdio.h>

#include "input.h"

static constexpr double keyboard_animation_ms = 160.0;
static constexpr double reset_animation_ms = 220.0;
static constexpr double pi = 3.14159265358979323846;

static double logical_coordinate(
  double value, uint32_t window_size, uint32_t logical_size
) {
  return window_size == 0 ? value
                          : value * (double)logical_size / (double)window_size;
}

static mln_screen_point logical_point(double x, double y, viewport value) {
  return (mln_screen_point){
    .x = logical_coordinate(x, value.window_width, value.logical_width),
    .y = logical_coordinate(y, value.window_height, value.logical_height),
  };
}

static mln_animation_options animation(double duration_ms) {
  mln_animation_options options = mln_animation_options_default();
  options.fields = MLN_ANIMATION_OPTION_DURATION;
  options.duration_ms = duration_ms;
  return options;
}

static input_result submitted(app_error error) {
  return (input_result){
    .handled = true, .camera_changed = error == APP_OK, .error = error
  };
}

static app_error submit_camera(
  map_state* state, const mln_camera_options* camera, uint32_t mode,
  double duration_ms, uint32_t gesture_phase, uint64_t gesture_id
) {
  mln_animation_options options = animation(duration_ms);
  return map_state_update_camera(
    state, camera, mode, mode == MLN_CAMERA_UPDATE_MODE_JUMP ? NULL : &options,
    gesture_phase, gesture_id
  );
}

static app_error pan(
  map_state* state, double dx, double dy, uint32_t mode, double duration_ms,
  uint64_t gesture_id
) {
  mln_camera_options current;
  app_error error = map_state_camera_query(state, &current);
  if (error != APP_OK) return error;

  const double latitude =
    fmax(-85.05112878, fmin(85.05112878, current.latitude));
  const double latitude_radians = latitude * pi / 180.0;
  double x = (current.longitude + 180.0) / 360.0;
  double y =
    (1.0 - log(tan(latitude_radians) + 1.0 / cos(latitude_radians)) / pi) / 2.0;
  const double world_size = 512.0 * exp2(current.zoom);
  const double bearing = current.bearing * pi / 180.0;
  x += (-dx * cos(bearing) - dy * sin(bearing)) / world_size;
  y += (dx * sin(bearing) - dy * cos(bearing)) / world_size;

  mln_camera_options target = mln_camera_options_default();
  target.fields = MLN_CAMERA_OPTION_CENTER;
  target.longitude = x * 360.0 - 180.0;
  target.latitude = 180.0 / pi * atan(sinh(pi * (1.0 - 2.0 * y)));
  return submit_camera(
    state, &target, mode, duration_ms, MLN_GESTURE_PHASE_UPDATE, gesture_id
  );
}

static app_error zoom(
  map_state* state, double scale, mln_screen_point anchor, uint32_t mode,
  double duration_ms, uint64_t gesture_id
) {
  mln_camera_options current;
  app_error error = map_state_camera_query(state, &current);
  if (error != APP_OK) return error;
  mln_camera_options target = mln_camera_options_default();
  target.fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_ANCHOR;
  target.zoom = current.zoom + log2(scale);
  target.anchor = anchor;
  return submit_camera(
    state, &target, mode, duration_ms, MLN_GESTURE_PHASE_UPDATE, gesture_id
  );
}

static app_error adjust_orientation(
  map_state* state, double bearing_delta, double pitch_delta, uint32_t mode,
  double duration_ms, uint64_t gesture_id
) {
  mln_camera_options current;
  app_error error = map_state_camera_query(state, &current);
  if (error != APP_OK) return error;
  mln_camera_options target = mln_camera_options_default();
  target.fields = MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  target.bearing = current.bearing + bearing_delta;
  target.pitch = fmax(0.0, fmin(60.0, current.pitch + pitch_delta));
  return submit_camera(
    state, &target, mode, duration_ms, MLN_GESTURE_PHASE_UPDATE, gesture_id
  );
}

static drag_mode drag_mode_for_button(uint8_t button) {
  if (button == SDL_BUTTON_RIGHT) return DRAG_MODE_ROTATE;
  if (button != SDL_BUTTON_LEFT) return DRAG_MODE_NONE;
  return (SDL_GetModState() & SDL_KMOD_CTRL) != 0 ? DRAG_MODE_ROTATE
                                                  : DRAG_MODE_PAN;
}

static input_result handle_mouse_button_down(
  input_controller* controller, const SDL_MouseButtonEvent* button,
  map_state* state, viewport value
) {
  if (controller->drag_mode != DRAG_MODE_NONE)
    return (input_result){.handled = true};
  const drag_mode mode = drag_mode_for_button(button->button);
  if (mode == DRAG_MODE_NONE) return (input_result){};
  const mln_screen_point cursor = logical_point(button->x, button->y, value);
  controller->last_x = cursor.x;
  controller->last_y = cursor.y;
  controller->drag_mode = mode;
  controller->drag_button = button->button;
  controller->gesture_id = ++controller->next_gesture_id;
  mln_camera_options camera = mln_camera_options_default();
  const app_error error = map_state_update_camera(
    state, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, NULL, MLN_GESTURE_PHASE_BEGIN,
    controller->gesture_id
  );
  return submitted(error);
}

static input_result handle_mouse_button_up(
  input_controller* controller, const SDL_MouseButtonEvent* button,
  map_state* state
) {
  if (button->button != SDL_BUTTON_LEFT && button->button != SDL_BUTTON_RIGHT)
    return (input_result){};
  if (button->button != controller->drag_button)
    return (input_result){.handled = true};
  controller->drag_mode = DRAG_MODE_NONE;
  controller->drag_button = 0;
  mln_camera_options camera = mln_camera_options_default();
  const app_error error = map_state_update_camera(
    state, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, NULL, MLN_GESTURE_PHASE_END,
    controller->gesture_id
  );
  controller->gesture_id = 0;
  return (input_result){.handled = true, .error = error};
}

static input_result handle_mouse_motion(
  input_controller* controller, const SDL_MouseMotionEvent* motion,
  map_state* state, viewport value
) {
  if (controller->drag_mode == DRAG_MODE_NONE) return (input_result){};
  const mln_screen_point cursor = logical_point(motion->x, motion->y, value);
  const double dx = cursor.x - controller->last_x;
  const double dy = cursor.y - controller->last_y;
  controller->last_x = cursor.x;
  controller->last_y = cursor.y;
  if (dx == 0.0 && dy == 0.0) return (input_result){.handled = true};
  return submitted(
    controller->drag_mode == DRAG_MODE_PAN
      ? pan(
          state, dx, dy, MLN_CAMERA_UPDATE_MODE_JUMP, 0.0,
          controller->gesture_id
        )
      : adjust_orientation(
          state, dx * 0.5, dy / 2.0, MLN_CAMERA_UPDATE_MODE_JUMP, 0.0,
          controller->gesture_id
        )
  );
}

static input_result handle_mouse_wheel(
  const SDL_MouseWheelEvent* wheel, map_state* state, viewport value
) {
  if (wheel->y == 0.0) return (input_result){.handled = true};
  return submitted(zoom(
    state, pow(2.0, wheel->y * 0.25),
    logical_point(wheel->mouse_x, wheel->mouse_y, value),
    MLN_CAMERA_UPDATE_MODE_JUMP, 0.0, 0
  ));
}

static input_result handle_key_down(
  const SDL_KeyboardEvent* key, map_state* state, viewport value
) {
  constexpr double pan_step = 120.0;
  constexpr double zoom_step = 1.25;
  constexpr double bearing_step = 10.0;
  constexpr double pitch_step = 5.0;
  const mln_screen_point center = {
    .x = (double)value.logical_width / 2.0,
    .y = (double)value.logical_height / 2.0,
  };
  app_error error;
  switch (key->scancode) {
    case SDL_SCANCODE_LEFT:
    case SDL_SCANCODE_A:
      error = pan(
        state, pan_step, 0.0, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_RIGHT:
    case SDL_SCANCODE_D:
      error = pan(
        state, -pan_step, 0.0, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_UP:
    case SDL_SCANCODE_W:
      error = pan(
        state, 0.0, pan_step, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_S:
      error = pan(
        state, 0.0, -pan_step, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_EQUALS:
    case SDL_SCANCODE_KP_PLUS:
      error = zoom(
        state, zoom_step, center, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_MINUS:
    case SDL_SCANCODE_KP_MINUS:
      error = zoom(
        state, 1.0 / zoom_step, center, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_Q:
      error = adjust_orientation(
        state, -bearing_step, 0.0, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_E:
      error = adjust_orientation(
        state, bearing_step, 0.0, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_RIGHTBRACKET:
      error = adjust_orientation(
        state, 0.0, pitch_step, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_LEFTBRACKET:
      error = adjust_orientation(
        state, 0.0, -pitch_step, MLN_CAMERA_UPDATE_MODE_EASE,
        keyboard_animation_ms, 0
      );
      break;
    case SDL_SCANCODE_0: {
      mln_camera_options target = mln_camera_options_default();
      target.fields = MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
      target.bearing = 0.0;
      target.pitch = 0.0;
      error = submit_camera(
        state, &target, MLN_CAMERA_UPDATE_MODE_EASE, reset_animation_ms,
        MLN_GESTURE_PHASE_NONE, 0
      );
      break;
    }
    default:
      return (input_result){};
  }
  return submitted(error);
}

input_result input_controller_handle_event(
  input_controller* controller, const SDL_Event* event, map_state* state,
  viewport value
) {
  switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      return handle_mouse_button_down(controller, &event->button, state, value);
    case SDL_EVENT_MOUSE_BUTTON_UP:
      return handle_mouse_button_up(controller, &event->button, state);
    case SDL_EVENT_MOUSE_MOTION:
      return handle_mouse_motion(controller, &event->motion, state, value);
    case SDL_EVENT_MOUSE_WHEEL:
      return handle_mouse_wheel(&event->wheel, state, value);
    case SDL_EVENT_KEY_DOWN:
      return handle_key_down(&event->key, state, value);
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
