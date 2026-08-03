#include <string.h>

#include "types.h"

const char* app_error_name(app_error error) {
  switch (error) {
    case APP_OK:
      return "ok";
    case APP_ERROR_INVALID_ARGUMENTS:
      return "invalid arguments";
    case APP_ERROR_SDL_INIT_FAILED:
      return "SDL init failed";
    case APP_ERROR_WINDOW_CREATE_FAILED:
      return "window create failed";
    case APP_ERROR_RUNTIME_CREATE_FAILED:
      return "runtime create failed";
    case APP_ERROR_MAP_CREATE_FAILED:
      return "map create failed";
    case APP_ERROR_TEXTURE_ATTACH_FAILED:
      return "texture attach failed";
    case APP_ERROR_STYLE_LOAD_FAILED:
      return "style load failed";
    case APP_ERROR_CAMERA_JUMP_FAILED:
      return "camera jump failed";
    case APP_ERROR_CAMERA_COMMAND_FAILED:
      return "camera command failed";
    case APP_ERROR_TEXTURE_RESIZE_FAILED:
      return "texture resize failed";
    case APP_ERROR_TEXTURE_RENDER_FAILED:
      return "texture render failed";
    case APP_ERROR_SURFACE_ATTACH_FAILED:
      return "surface attach failed";
    case APP_ERROR_SURFACE_RESIZE_FAILED:
      return "surface resize failed";
    case APP_ERROR_SURFACE_RENDER_FAILED:
      return "surface render failed";
    case APP_ERROR_BACKEND_SETUP_FAILED:
      return "backend setup failed";
    case APP_ERROR_BACKEND_DRAW_FAILED:
      return "backend draw failed";
    case APP_ERROR_RENDER_BACKEND_MISMATCH:
      return "render backend mismatch";
    case APP_ERROR_WAKE_SOURCE_FAILED:
      return "wake source acquire failed";
    case APP_ERROR_RUNTIME_PUMP_FAILED:
      return "runtime pump failed";
    case APP_ERROR_EVENT_POLL_FAILED:
      return "event poll failed";
    case APP_ERROR_THREAD_SPAWN_FAILED:
      return "thread spawn failed";
  }
  return "unknown error";
}

bool render_target_mode_parse(const char* value, render_target_mode* out_mode) {
  if (strcmp(value, "owned-texture") == 0) {
    *out_mode = RENDER_TARGET_MODE_OWNED_TEXTURE;
    return true;
  }
  if (strcmp(value, "borrowed-texture") == 0) {
    *out_mode = RENDER_TARGET_MODE_BORROWED_TEXTURE;
    return true;
  }
  if (strcmp(value, "native-surface") == 0) {
    *out_mode = RENDER_TARGET_MODE_NATIVE_SURFACE;
    return true;
  }
  return false;
}

const char* render_target_mode_label(render_target_mode mode) {
  switch (mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return "owned-texture";
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return "borrowed-texture";
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return "native-surface";
  }
  return "unknown";
}

const char* render_target_mode_status_line(render_target_mode mode) {
  switch (mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return "samples MapLibre-owned texture frames into the host swapchain";
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return "renders into a host-owned texture, then samples it into the host "
             "swapchain";
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return "renders directly to the host window surface";
  }
  return "unknown";
}
