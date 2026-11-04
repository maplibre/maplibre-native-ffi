#include <cstdint>
#include <memory>
#include <utility>

#include <native_window/external_window.h>

#include "render/ohos_surface_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/vulkan_device.hpp"

namespace mln::core {
namespace {

// Takes a reference to the window that `window` or `surface_id` names. A
// window found by its surface ID is a reference of its own, which
// OH_NativeWindow_DestroyNativeWindow() ends.
auto acquire_window(void* window, std::uint64_t surface_id, OhosWindow& out)
  -> mln_status {
  auto* native = static_cast<OHNativeWindow*>(window);
  if (native != nullptr) {
    if (OH_NativeWindow_NativeObjectReference(native) != 0) {
      set_thread_error("window is not an OHNativeWindow");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    // The deleter runs even when allocating the control block throws.
    out = OhosWindow{native, [](OHNativeWindow* owned) {
                       OH_NativeWindow_NativeObjectUnreference(owned);
                     }};
    return MLN_STATUS_OK;
  }
  if (
    OH_NativeWindow_CreateNativeWindowFromSurfaceId(surface_id, &native) != 0 ||
    native == nullptr
  ) {
    set_thread_error("no surface in this process has surface_id");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out = OhosWindow{native, &OH_NativeWindow_DestroyNativeWindow};
  return MLN_STATUS_OK;
}

}  // namespace

auto ohos_surface_attach_start(
  mln_map map, const mln_ohos_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) return map_status;
  const auto descriptor_status = validate_ohos_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) return descriptor_status;
  auto context = std::shared_ptr<const mln_vulkan_context_descriptor>{};
  const auto device_status = window_presentation_device(device, context);
  if (device_status != MLN_STATUS_OK) return device_status;

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  const auto size_status = size_new_session(*session, descriptor->logical_size);
  if (size_status != MLN_STATUS_OK) return size_status;
  const auto driver_status = require_render_driver(
    options, MLN_RENDER_DRIVER_CORE_WORKER,
    "an OpenHarmony surface session runs on a library thread, since the "
    "library owns its graphics context; attach it with "
    "MLN_RENDER_DRIVER_CORE_WORKER"
  );
  if (driver_status != MLN_STATUS_OK) return driver_status;
  auto window = OhosWindow{};
  const auto window_status =
    acquire_window(descriptor->window, descriptor->surface_id, window);
  if (window_status != MLN_STATUS_OK) return window_status;

  session->target_identity = {render_target_handle(window.get())};
#if defined(MLN_RENDER_BACKEND_VULKAN)
  session->accepts_queue_lock = true;
#endif
  // The backend takes the window from here, so that this closure, which the
  // session keeps, doesn't hold it past a release of the target.
  session->initialize_backend =
    [attach = OhosSurfaceAttach{
       .window = std::move(window), .device = std::move(context)
     }](mln_render_session_object& target) mutable {
      return create_ohos_surface_backend(std::move(attach), target);
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_PRESENTATION,
    .producer_sync_kind = {},
    .consumer_sync_kinds = 0,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Surface, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_ohos_surface>()
  );
}

auto ohos_surface_set_target_start(
  mln_render_session session, const mln_ohos_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  const auto replacement_status =
    validate_ohos_surface_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) return replacement_status;
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::Surface, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) return submission_status;
  auto window = OhosWindow{};
  const auto window_status =
    acquire_window(replacement->window, replacement->surface_id, window);
  if (window_status != MLN_STATUS_OK) return window_status;
  auto identity = RenderTargetIdentity{render_target_handle(window.get())};
  return enqueue_surface_retarget(
    session, replacement->logical_size, std::move(identity),
    [window = std::move(window)](
      mln_render_session_object& live, mln::Size physical_size
    ) { return live.surface.backend->set_ohos_target(window, physical_size); },
    std::move(checked_completion),
    valueless_completion<&mln_render_session_set_ohos_surface_target>()
  );
}

}  // namespace mln::core
