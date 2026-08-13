// Shared value types: the app error set, the viewport snapshot, and the
// render-target mode selected on the command line.

#ifndef C_MAP_TYPES_H
#define C_MAP_TYPES_H

#include <stdint.h>

typedef enum app_error : uint8_t {
  APP_OK = 0,
  APP_ERROR_RUNTIME_CREATE_FAILED,
  APP_ERROR_MAP_CREATE_FAILED,
  APP_ERROR_TEXTURE_ATTACH_FAILED,
  APP_ERROR_STYLE_LOAD_FAILED,
  APP_ERROR_CAMERA_JUMP_FAILED,
  APP_ERROR_CAMERA_COMMAND_FAILED,
  APP_ERROR_TEXTURE_RESIZE_FAILED,
  APP_ERROR_TEXTURE_RENDER_FAILED,
  APP_ERROR_SURFACE_ATTACH_FAILED,
  APP_ERROR_SURFACE_RESIZE_FAILED,
  APP_ERROR_SURFACE_RENDER_FAILED,
  APP_ERROR_BACKEND_SETUP_FAILED,
  APP_ERROR_BACKEND_DRAW_FAILED,
  APP_ERROR_RENDER_BACKEND_MISMATCH,
  APP_ERROR_WAKE_SOURCE_FAILED,
  APP_ERROR_RUNTIME_PUMP_FAILED,
  APP_ERROR_EVENT_MASK_FAILED,
  APP_ERROR_EVENT_DRAIN_FAILED,
  APP_ERROR_THREAD_SPAWN_FAILED,
} app_error;

const char* app_error_name(app_error error);

/// One snapshot of the window's size in every unit the example needs: logical
/// map pixels, SDL window coordinates, and physical device pixels.
typedef struct viewport {
  uint32_t logical_width;
  uint32_t logical_height;
  uint32_t window_width;
  uint32_t window_height;
  uint32_t physical_width;
  uint32_t physical_height;
  double scale_factor;
} viewport;

typedef enum render_target_mode : uint8_t {
  RENDER_TARGET_MODE_OWNED_TEXTURE,
  RENDER_TARGET_MODE_BORROWED_TEXTURE,
  RENDER_TARGET_MODE_NATIVE_SURFACE,
} render_target_mode;

/// Returns whether value named a mode, writing the mode when it did.
bool render_target_mode_parse(const char* value, render_target_mode* out_mode);
const char* render_target_mode_label(render_target_mode mode);
const char* render_target_mode_status_line(render_target_mode mode);

#endif  // C_MAP_TYPES_H
