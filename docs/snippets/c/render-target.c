// Attaching each kind of OpenGL render target to a live map, replacing the
// target of an attached session, and closing the two in order.

#include <maplibre_native_c.h>

static mln_render_session_attach_options caller_driver(void) {
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  return options;
}

mln_status attach_to_window(
  mln_map map, const mln_opengl_context_descriptor* context, void* egl_surface,
  uint32_t width, uint32_t height, double scale_factor,
  mln_render_session* out_session, const mln_completion* completion
) {
  // #region surface
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  descriptor.surface = egl_surface;
  const mln_render_session_attach_options options = caller_driver();
  return mln_opengl_surface_attach(
    map, &descriptor, &options, out_session, completion
  );
  // #endregion surface
}

mln_status attach_to_own_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t width,
  uint32_t height, double scale_factor, mln_render_session* out_session,
  const mln_completion* completion
) {
  // #region owned
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  const mln_render_session_attach_options options = caller_driver();
  return mln_opengl_owned_texture_attach(
    map, &descriptor, &options, out_session, completion
  );
  // #endregion owned
}

mln_status attach_to_host_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t texture,
  uint32_t texture_target, uint32_t logical_width, uint32_t logical_height,
  double scale_factor, mln_render_session* out_session,
  const mln_completion* completion
) {
  // #region borrowed
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  descriptor.extent.width = logical_width;
  descriptor.extent.height = logical_height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.physical_width = (uint32_t)(logical_width * scale_factor);
  descriptor.physical_height = (uint32_t)(logical_height * scale_factor);
  descriptor.context = *context;
  descriptor.texture = texture;
  descriptor.target = texture_target;
  const mln_render_session_attach_options options = caller_driver();
  return mln_opengl_borrowed_texture_attach(
    map, &descriptor, &options, out_session, completion
  );
  // #endregion borrowed
}

mln_status resize_session(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor, const mln_completion* completion
) {
  // #region resize
  const mln_render_target_extent extent = {
    .width = width, .height = height, .scale_factor = scale_factor
  };
  return mln_render_session_resize(session, &extent, completion);
  // #endregion resize
}

mln_status resize_window_target(
  mln_render_session session, const mln_opengl_context_descriptor* context,
  void* egl_surface, uint32_t width, uint32_t height, double scale_factor,
  const mln_completion* completion
) {
  // #region set-target
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  descriptor.surface = egl_surface;
  return mln_opengl_surface_set_target(session, &descriptor, completion);
  // #endregion set-target
}

typedef struct teardown_state {
  mln_map map;
  mln_render_session session;
} teardown_state;

static void map_released(void* user_data, const mln_completion_result* result) {
  (void)user_data;
  (void)result;
}

static void detached(void* user_data, const mln_completion_result* result) {
  teardown_state* state = user_data;
  if (result->status != MLN_STATUS_OK) return;
  mln_render_session_destroy(state->session);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = map_released,
  };
  (void)mln_map_release(state->map, &completion);
}

mln_status release_map(
  mln_map map, mln_render_session session, teardown_state* state
) {
  // #region teardown
  state->map = map;
  state->session = session;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = detached,
    .user_data = state,
  };
  return mln_render_session_detach(session, &completion);
  // #endregion teardown
}
