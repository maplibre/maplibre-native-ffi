// A process that exits while a Metal core worker renders a session-owned
// texture, with its runtime, map, resource provider, wakes, and log callback
// all live. See render_probe.h.

#include "render_probe.h"

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  mln_metal_owned_texture_descriptor descriptor =
    mln_metal_owned_texture_descriptor_default();
  descriptor.extent = probe_extent;
  descriptor.context.device = context->metal_device;
  return mln_metal_owned_texture_attach(
    map, &descriptor, options, out_session, completion, NULL
  );
}

int main(void) {
  return probe_exit_while_rendering(MLN_TEST_GRAPHICS_BACKEND_METAL, attach);
}
