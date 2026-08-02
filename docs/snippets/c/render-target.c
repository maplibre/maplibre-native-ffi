// Attaching each kind of render target to a live map, replacing the target of
// an attached session, and releasing the two in order. OpenGL through EGL here;
// Metal and Vulkan use their own descriptor types and attach functions.

#include <maplibre_native_c.h>

mln_render_session attach_to_window(
  mln_map map, const mln_opengl_context_descriptor* context, void* egl_surface,
  uint32_t width, uint32_t height, double scale_factor
) {
  // #region surface
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  descriptor.surface = egl_surface;

  mln_render_session session = MLN_HANDLE_NULL;
  const mln_status status =
    mln_opengl_surface_attach(map, &descriptor, &session);
  // #endregion surface

  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

mln_render_session attach_to_own_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t width,
  uint32_t height, double scale_factor
) {
  // #region owned
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;

  mln_render_session session = MLN_HANDLE_NULL;
  const mln_status status =
    mln_opengl_owned_texture_attach(map, &descriptor, &session);
  // #endregion owned

  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

mln_render_session attach_to_host_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t texture,
  uint32_t texture_target, uint32_t logical_width, uint32_t logical_height,
  double scale_factor
) {
  // #region borrowed
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  descriptor.extent.width = logical_width;
  descriptor.extent.height = logical_height;
  descriptor.extent.scale_factor = scale_factor;

  // Stated rather than derived from the logical extent. These must equal the
  // texture's level-0 dimensions, which the session cannot verify on ES 3.0.
  descriptor.physical_width = (uint32_t)(logical_width * scale_factor);
  descriptor.physical_height = (uint32_t)(logical_height * scale_factor);

  descriptor.context = *context;
  descriptor.texture = texture;
  descriptor.target = texture_target;  // GL_TEXTURE_2D

  mln_render_session session = MLN_HANDLE_NULL;
  const mln_status status =
    mln_opengl_borrowed_texture_attach(map, &descriptor, &session);
  // #endregion borrowed

  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

mln_status resize_session(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor
) {
  // #region resize
  // Surface and session-owned texture targets resize in place. A borrowed
  // texture reports an unsupported status here, because its size comes from
  // the texture that the host allocated.
  return mln_render_session_resize(session, width, height, scale_factor);
  // #endregion resize
}

mln_status resize_window_target(
  mln_render_session session, const mln_opengl_context_descriptor* context,
  void* egl_surface, uint32_t width, uint32_t height, double scale_factor
) {
  // #region set-target
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  descriptor.surface = egl_surface;

  return mln_opengl_surface_set_target(session, &descriptor);
  // #endregion set-target
}

void release_map(mln_map map, mln_render_session session) {
  // #region teardown
  mln_render_session_destroy(session);
  mln_map_destroy(map);
  // #endregion teardown
}
