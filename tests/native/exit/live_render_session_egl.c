// A process that exits after abandoning two OpenGL sessions mid-frame: an
// offscreen session on a core worker in a context of its own, and one on a host
// graphics thread that renders into a texture of the host's context. Its
// runtime, maps, session handles, resource provider, wakes, and log callback
// all stay live. See render_probe.h.

#include "render_probe.h"

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_test_graphics_texture_info* texture,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  // A borrowed OpenGL texture needs the host's graphics thread, so the core
  // worker renders offscreen instead.
  if (options->driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    const mln_render_target target = {
      .size = sizeof(mln_render_target),
      .kind = MLN_RENDER_TARGET_KIND_OFFSCREEN,
      .data.offscreen = {.logical_size = probe_size},
    };
    return mln_map_attach(
      map, &target, MLN_HANDLE_NULL, options, out_session, completion, NULL
    );
  }
  const mln_open_gl_borrowed_texture entry = {
    .texture = texture->opengl_texture
  };
  mln_open_gl_borrowed_texture_descriptor descriptor = {
    .size = sizeof(mln_open_gl_borrowed_texture_descriptor)
  };
  descriptor.logical_size = probe_size;
  descriptor.physical_width = texture->width;
  descriptor.physical_height = texture->height;
  descriptor.context = (mln_open_gl_context_descriptor){
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL,
    .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED,
    .data = {
      .egl = {
        .display = context->egl_display,
        .config = context->egl_config,
        .share_context = context->egl_context,
      }
    },
  };
  descriptor.textures = &entry;
  descriptor.texture_count = 1;
  descriptor.target = texture->opengl_target;
  return mln_map_attach_open_gl_borrowed_texture(
    map, &descriptor, options, out_session, completion, NULL
  );
}

int main(void) {
  return probe_exit_after_abandoning(MLN_TEST_GRAPHICS_BACKEND_EGL, attach);
}
