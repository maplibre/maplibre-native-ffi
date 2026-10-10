// A process that exits after abandoning two OpenGL sessions mid-frame: one on
// a core worker in a dedicated EGL context, and one on a host graphics thread
// in the host's context. Its runtime, maps, session handles, resource
// provider, wakes, and log callback all stay live. See render_probe.h.

#include "render_probe.h"

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent = probe_extent;
  // A core worker renders in a context of its own; a host graphics thread
  // renders in the host's context.
  const bool dedicated = options->driver == MLN_RENDER_DRIVER_CORE_WORKER;
  descriptor.context = (mln_opengl_context_descriptor){
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .ownership = dedicated ? MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED
                           : MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED,
    .data = {
      .egl = {
        .display = context->egl_display,
        .config = context->egl_config,
        .share_context = dedicated ? NULL : context->egl_context,
        .client_api = MLN_OPENGL_CLIENT_API_GLES,
      }
    },
  };
  return mln_opengl_owned_texture_attach(
    map, &descriptor, options, out_session, completion, NULL
  );
}

int main(void) {
  return probe_exit_after_abandoning(MLN_TEST_GRAPHICS_BACKEND_EGL, attach);
}
