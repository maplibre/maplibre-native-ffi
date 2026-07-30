// Attaches an EGL surface the host window toolkit already created. Metal and
// Vulkan use their own descriptor type and attach function, carrying their own
// handles.

#include <maplibre_native_c.h>

mln_render_session attach_surface(
  mln_map map, void* egl_display, void* egl_config, void* egl_share_context,
  void* egl_surface, uint32_t logical_width, uint32_t logical_height,
  double scale_factor
) {
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();

  // Logical pixels, the same units the camera and hit-testing use.
  descriptor.extent.width = logical_width;
  descriptor.extent.height = logical_height;
  descriptor.extent.scale_factor = scale_factor;

  // The session joins your context's share group.
  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor.context.data.egl.display = egl_display;
  descriptor.context.data.egl.config = egl_config;
  descriptor.context.data.egl.share_context = egl_share_context;

  descriptor.surface = egl_surface;

  mln_render_session session = MLN_HANDLE_NULL;
  if (mln_opengl_surface_attach(map, &descriptor, &session) != MLN_STATUS_OK) {
    return MLN_HANDLE_NULL;
  }
  return session;
}
