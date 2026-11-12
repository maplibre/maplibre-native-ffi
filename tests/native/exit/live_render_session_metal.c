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
  const mln_metal_device_descriptor host = {
    .size = sizeof(mln_metal_device_descriptor),
    .device = context->metal_device,
  };
  mln_gpu_device device = MLN_HANDLE_NULL;
  const mln_status wrapped = mln_gpu_device_wrap_metal(&host, &device, NULL);
  if (wrapped != MLN_STATUS_OK) return wrapped;
  const mln_metal_borrowed_texture entry = {.texture = texture->metal_texture};
  const mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_METAL_BORROWED_TEXTURES,
    .data.metal_borrowed_textures = {
      .logical_size = probe_size,
      .physical_width = texture->width,
      .physical_height = texture->height,
      .textures = &entry,
      .texture_count = 1,
    },
  };
  const mln_status status = mln_map_attach(
    map, &target, device, options, out_session, completion, NULL
  );
  mln_gpu_device_release(device);
  return status;
}

int main(void) {
  return probe_exit_after_abandoning(MLN_TEST_GRAPHICS_BACKEND_METAL, attach);
}
