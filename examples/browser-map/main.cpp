#include <emscripten.h>
#include <emscripten/html5.h>

#include <cstdint>
#include <cstdio>

#include "maplibre_native_c.h"

namespace {

constexpr auto k_width = uint32_t{800};
constexpr auto k_height = uint32_t{600};
constexpr auto k_style_url = "https://tiles.openfreemap.org/styles/bright";

mln_runtime* g_runtime = nullptr;
mln_map* g_map = nullptr;
mln_render_session* g_session = nullptr;

auto log_status(const char* stage, mln_status status) -> bool {
  if (status == MLN_STATUS_OK) {
    return true;
  }
  const char* message = mln_thread_last_error_message();
  if (message == nullptr || message[0] == '\0') {
    message = "unknown error";
  }
  std::fprintf(
    stderr, "%s failed (%d): %s\n", stage, static_cast<int>(status), message
  );
  return false;
}

void render_frame() {
  if (g_runtime != nullptr) {
    mln_runtime_run_once(g_runtime);
  }
  if (g_map == nullptr || g_session == nullptr) {
    return;
  }

  const auto repaint_status = mln_map_request_repaint(g_map);
  if (repaint_status != MLN_STATUS_OK) {
    return;
  }

  const auto render_status = mln_render_session_render_update(g_session);
  if (render_status != MLN_STATUS_OK) {
    return;
  }
}

auto animation_frame(double, void*) -> EM_BOOL {
  render_frame();
  return EM_TRUE;
}

void resize_canvas() {
  emscripten_set_canvas_element_size("#canvas", k_width, k_height);
  if (g_session != nullptr) {
    log_status(
      "resize",
      mln_render_session_resize(g_session, k_width, k_height, 1.0)
    );
  }
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
auto mln_browser_map_init(void) -> int32_t {
  if ((mln_supported_render_backend_mask() & MLN_RENDER_BACKEND_FLAG_WEBGPU) == 0) {
    std::fprintf(stderr, "WebGPU backend is not available in this build\n");
    return 1;
  }

  resize_canvas();

  auto runtime_options = mln_runtime_options_default();
  runtime_options.cache_path = ":memory:";

  const auto runtime_status = mln_runtime_create(&runtime_options, &g_runtime);
  if (!log_status("runtime_create", runtime_status)) {
    return 1;
  }

  auto map_options = mln_map_options_default();
  map_options.width = k_width;
  map_options.height = k_height;
  map_options.scale_factor = 1.0;
  map_options.map_mode = MLN_MAP_MODE_CONTINUOUS;

  const auto map_status = mln_map_create(g_runtime, &map_options, &g_map);
  if (!log_status("map_create", map_status)) {
    return 1;
  }

  auto surface_desc = mln_webgpu_surface_descriptor_default();
  surface_desc.extent.width = k_width;
  surface_desc.extent.height = k_height;
  surface_desc.canvas_selector = "#canvas";

  const auto attach_status =
    mln_webgpu_surface_attach(g_map, &surface_desc, &g_session);
  if (!log_status("surface_attach", attach_status)) {
    return 1;
  }

  auto camera = mln_camera_options_default();
  camera.fields =
    MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_BEARING |
    MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 40.7128;
  camera.longitude = -74.0060;
  camera.zoom = 11.0;
  camera.bearing = 0.0;
  camera.pitch = 0.0;

  if (!log_status("jump_to", mln_map_jump_to(g_map, &camera))) {
    return 1;
  }

  if (!log_status("set_style_url", mln_map_set_style_url(g_map, k_style_url))) {
    return 1;
  }

  emscripten_request_animation_frame_loop(animation_frame, nullptr);
  return 0;
}

}  // extern "C"

int main() { return mln_browser_map_init(); }
