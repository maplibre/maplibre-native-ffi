// A process that exits while a Vulkan core worker renders a session-owned
// texture, with its runtime, map, resource provider, wakes, and log callback
// all live. See render_probe.h.

#include "render_probe.h"

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
  descriptor.extent = probe_extent;
  descriptor.context.instance = context->vulkan_instance;
  descriptor.context.physical_device = context->vulkan_physical_device;
  descriptor.context.device = context->vulkan_device;
  descriptor.context.graphics_queue = context->vulkan_queue;
  descriptor.context.graphics_queue_family_index =
    context->vulkan_queue_family_index;
  descriptor.context.get_instance_proc_addr =
    context->vulkan_get_instance_proc_addr;
  descriptor.context.get_device_proc_addr =
    context->vulkan_get_device_proc_addr;
  return mln_vulkan_owned_texture_attach(
    map, &descriptor, options, out_session, completion, NULL
  );
}

int main(void) {
  return probe_exit_while_rendering(MLN_TEST_GRAPHICS_BACKEND_VULKAN, attach);
}
