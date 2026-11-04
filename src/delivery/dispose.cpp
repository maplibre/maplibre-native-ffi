#include "delivery/dispose.hpp"

#include "c_api/boundary.hpp"
#include "delivery/delivery_queue.hpp"
#include "geojson/geojson_source_data.hpp"
#include "map/map.hpp"
#include "render/frame_pacer.hpp"
#include "render/offscreen_device.hpp"
#include "render/render_session_common.hpp"
#include "render/vulkan_device.hpp"
#include "resources/custom_resource_provider.hpp"
#include "resources/resource_transform.hpp"
#include "runtime/runtime.hpp"
#include "style/style_expression.hpp"

namespace mln::delivery {

auto dispose_owner(core::HandleKind kind, std::uint64_t handle) noexcept
  -> mln_status {
  auto dispose = [&]() -> mln_status {
    switch (kind) {
      case core::HandleKind::AcquiredFrame:
        return core::acquired_frame_discard(handle);
      case core::HandleKind::DeliveryQueue:
        release_delivery_queue(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::FramePacer:
        core::frame_pacer_release(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::GeoJsonSourceData:
        core::geojson_source_data_release(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::OffscreenDevice:
        core::offscreen_device_release(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::Map:
        return core::dispose_map(handle);
      case core::HandleKind::MapProjection:
        core::map_projection_release(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::RenderFrameBatch:
        core::render_frame_batch_release(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::RenderSession:
        return core::render_session_dispose(handle);
      case core::HandleKind::ResourceRequest:
        core::release_resource_request(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::ResourceTransformRequest:
        core::release_resource_transform_request(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::Runtime:
        return core::dispose_runtime(handle);
      case core::HandleKind::StyleExpression:
        core::style_expression_release(handle);
        return MLN_STATUS_OK;
      case core::HandleKind::VulkanDevice:
        core::vulkan_device_release(handle);
        return MLN_STATUS_OK;
    }
    return MLN_STATUS_INVALID_ARGUMENT;
  };
  if (kind == core::HandleKind::Map || kind == core::HandleKind::Runtime)
    return c_api::native_boundary<false>(nullptr, dispose);
  return c_api::native_boundary(nullptr, dispose);
}

}  // namespace mln::delivery
