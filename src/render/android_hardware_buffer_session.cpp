#include <memory>
#include <utility>

#include "render/android_hardware_buffer_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "platform/android/hardware_buffer.hpp"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace mln::core {

auto android_hardware_buffer_attach_start(
  mln_map map, const mln_android_hardware_buffer_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) return map_status;
  const auto descriptor_status =
    validate_android_hardware_buffer_ring_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) return descriptor_status;
  const auto request_status =
    validate_render_session_attach_request(options, out_session, completion);
  if (request_status != MLN_STATUS_OK) return request_status;
  const auto driver_status = require_render_driver(
    options, MLN_RENDER_DRIVER_CORE_WORKER,
    "a hardware buffer ring renders on a library thread, since the library "
    "owns its graphics context; attach it with MLN_RENDER_DRIVER_CORE_WORKER"
  );
  if (driver_status != MLN_STATUS_OK) return driver_status;
  if (platform::android::hardware_buffer_functions() == nullptr) {
    set_thread_error(
      "Android hardware buffers need Android 8.0, API level 26, or later"
    );
    return MLN_STATUS_UNSUPPORTED;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  const auto size_status = size_new_session(*session, descriptor->logical_size);
  if (size_status != MLN_STATUS_OK) return size_status;
  session->texture.mode = TextureSessionMode::Exported;
  const auto ring = AndroidHardwareBufferRing{
    .buffer_count = descriptor->buffer_count,
    .usage = descriptor->usage,
  };
  const auto configure_status =
    configure_android_hardware_buffer_session(ring, device, *session);
  if (configure_status != MLN_STATUS_OK) return configure_status;
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = descriptor->buffer_count,
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION,
    .producer_sync_kind = {},
    .consumer_sync_kinds = MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_android_hardware_buffer_ring>()
  );
}

}  // namespace mln::core
