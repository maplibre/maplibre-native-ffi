// A process that exits after abandoning two Metal sessions mid-frame, one on a
// core worker and one on a host graphics thread, with its runtime, maps,
// session handles, resource provider, wakes, and log callback all live. See
// render_probe.h.

#include "render_probe.h"

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_test_graphics_texture_info* texture,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  (void)context;
  const mln_metal_borrowed_texture entry = {.texture = texture->metal_texture};
  mln_metal_borrowed_texture_descriptor descriptor = {
    .size = sizeof(mln_metal_borrowed_texture_descriptor)
  };
  descriptor.extent = probe_extent;
  descriptor.physical_width = texture->width;
  descriptor.physical_height = texture->height;
  descriptor.textures = &entry;
  descriptor.texture_count = 1;
  return mln_map_attach_metal_borrowed_texture(
    map, &descriptor, options, out_session, completion, NULL
  );
}

int main(void) {
  return probe_exit_after_abandoning(MLN_TEST_GRAPHICS_BACKEND_METAL, attach);
}
