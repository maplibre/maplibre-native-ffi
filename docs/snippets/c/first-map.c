// A windowed map on one thread: create a runtime, create a map, load a style,
// attach the window's surface, then pump and render until the window closes.
// OpenGL ES through EGL on Linux; other backends change only the attach call.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <stdint.h>

// Your window toolkit provides these. Open a window with an EGL surface, make
// the context current on this thread, then call run_map() with the handles.
extern void host_window_poll_events(void);
extern bool host_window_should_close(void);

// Every step either succeeds or jumps to the single teardown path at the
// bottom of run_map().
#define TRY(call)                \
  if ((call) != MLN_STATUS_OK) { \
    goto teardown;               \
  }

// The finished-frame event carries needs_repaint when MapLibre has more work
// for the next frame. Animations and label placement settle over those frames.
static bool asks_for_another_frame(const mln_runtime_event* event) {
  if (event->type != MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) return false;
  if (event->payload_type != MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME) {
    return false;
  }
  if (event->payload_size < sizeof(mln_runtime_event_render_frame)) {
    return false;
  }
  const mln_runtime_event_render_frame* frame = event->payload;
  return frame->needs_repaint;
}

int run_map(
  void* egl_display, void* egl_config, void* egl_context, void* egl_surface,
  uint32_t logical_width, uint32_t logical_height, double scale_factor
) {
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_render_session session = MLN_HANDLE_NULL;
  int exit_code = 1;

  // An in-memory cache database, so the run leaves nothing on disk.
  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.cache_path = ":memory:";

  // Logical pixels and the display's scale factor, the same units the camera
  // and hit-testing use.
  mln_map_options map_options = mln_map_options_default();
  map_options.width = logical_width;
  map_options.height = logical_height;
  map_options.scale_factor = scale_factor;

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 13.0;

  // The session creates its own OpenGL context in your context's share group,
  // and presents with eglSwapBuffers() on the surface named here.
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = logical_width;
  descriptor.extent.height = logical_height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor.context.data.egl.display = egl_display;
  descriptor.context.data.egl.config = egl_config;
  descriptor.context.data.egl.share_context = egl_context;
  descriptor.surface = egl_surface;

  TRY(mln_runtime_create(&runtime_options, &runtime));
  TRY(mln_map_create(runtime, &map_options, &map));
  TRY(
    mln_map_set_style_url(map, "https://tiles.openfreemap.org/styles/bright")
  );
  TRY(mln_map_jump_to(map, &camera));
  TRY(mln_opengl_surface_attach(map, &descriptor, &session));

  bool pending = true;
  while (!host_window_should_close()) {
    host_window_poll_events();

    // Waits until the runtime has work or 8 ms pass, then runs everything
    // that the map queued: style parsing, tile loads, transitions.
    mln_runtime_pump(runtime, 8);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (
        event.type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE ||
        asks_for_another_frame(&event)
      ) {
        pending = true;
      }
    }

    if (!pending) continue;

    // False means that the map has published no update for this session's
    // extent yet. Stay pending and try again on the next turn.
    bool rendered = false;
    if (mln_render_session_render_update(session, &rendered) == MLN_STATUS_OK) {
      pending = !rendered;
    }
  }
  exit_code = 0;

teardown:
  // The reverse of creation: destroy the session before the map, and the map
  // before the runtime.
  mln_render_session_destroy(session);
  mln_map_destroy(map);
  mln_runtime_destroy(runtime);
  return exit_code;
}
