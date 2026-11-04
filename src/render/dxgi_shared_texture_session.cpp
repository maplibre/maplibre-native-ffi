#include <memory>
#include <utility>

#include "render/dxgi_shared_texture_session.hpp"

#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace mln::core {

auto dxgi_shared_texture_attach_start(
  mln_map map, const mln_dxgi_shared_texture_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) return map_status;
  const auto descriptor_status =
    validate_dxgi_shared_texture_ring_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) return descriptor_status;
  const auto request_status =
    validate_render_session_attach_request(options, out_session, completion);
  if (request_status != MLN_STATUS_OK) return request_status;
  const auto driver_status = require_render_driver(
    options, MLN_RENDER_DRIVER_CORE_WORKER,
    "a DXGI shared texture ring renders on a library thread, since the "
    "library owns its graphics context; attach it with "
    "MLN_RENDER_DRIVER_CORE_WORKER"
  );
  if (driver_status != MLN_STATUS_OK) return driver_status;

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  const auto size_status = size_new_session(*session, descriptor->logical_size);
  if (size_status != MLN_STATUS_OK) return size_status;
  session->texture.mode = TextureSessionMode::Exported;
  const auto ring =
    DxgiSharedTextureRing{.texture_count = descriptor->texture_count};
  const auto configure_status =
    configure_dxgi_shared_texture_session(ring, device, *session);
  if (configure_status != MLN_STATUS_OK) return configure_status;
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = descriptor->texture_count,
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION,
    .producer_sync_kind = {},
    .consumer_sync_kinds = MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_dxgi_shared_texture_ring>()
  );
}

}  // namespace mln::core
