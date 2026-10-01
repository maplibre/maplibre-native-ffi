// A process that exits while an OpenGL core worker renders a session-owned
// texture in a dedicated EGL context, with its runtime, map, resource
// provider, wakes, and log callback all live. See render_probe.h.

#include "render_probe.h"

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent = probe_extent;
  descriptor.context = (mln_opengl_context_descriptor){
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED,
    .data = {
      .egl = {
        .size = sizeof(mln_egl_context_descriptor),
        .display = context->egl_display,
        .config = context->egl_config,
        .client_api = MLN_OPENGL_CLIENT_API_GLES,
      }
    },
  };
  return mln_opengl_owned_texture_attach(
    map, &descriptor, options, out_session, completion, NULL
  );
}

int main(void) {
  return probe_exit_while_rendering(MLN_TEST_GRAPHICS_BACKEND_EGL, attach);
}
