// Starts attachment of an EGL surface that the host window toolkit created.

#include <maplibre_native_c.h>

mln_status attach_surface_start(
  mln_map map, void* egl_display, void* egl_config, void* egl_share_context,
  void* egl_surface, uint32_t logical_width, uint32_t logical_height,
  double scale_factor, mln_render_session* out_session,
  mln_operation* out_operation
) {
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();

  // Logical pixels, the same units the camera and hit-testing use.
  descriptor.extent.width = logical_width;
  descriptor.extent.height = logical_height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor.context.data.egl.display = egl_display;
  descriptor.context.data.egl.config = egl_config;
  descriptor.context.data.egl.share_context = egl_share_context;
  descriptor.surface = egl_surface;

  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  return mln_opengl_surface_attach_start(
    map, &descriptor, &options, out_session, out_operation
  );
}
