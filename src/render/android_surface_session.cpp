#include <memory>
#include <utility>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include "render/android_surface_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/vulkan_device.hpp"

namespace mln::core {
namespace {

// Takes a reference to the window that `window`, or `jni_env` and `surface`,
// name. A Java Surface's window is found on the calling thread, whose JNIEnv
// the host passes.
auto acquire_window(
  void* window, void* jni_env, void* surface, AndroidWindow& out_window
) -> mln_status {
  auto* native = static_cast<ANativeWindow*>(window);
  if (native != nullptr) {
    ANativeWindow_acquire(native);
  } else {
    native = ANativeWindow_fromSurface(
      static_cast<JNIEnv*>(jni_env), static_cast<jobject>(surface)
    );
    if (native == nullptr) {
      set_thread_error("the Android surface has no window");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  // The deleter runs even when allocating the control block throws.
  out_window = AndroidWindow{native, &ANativeWindow_release};
  return MLN_STATUS_OK;
}

}  // namespace

auto android_surface_attach_start(
  mln_map map, const mln_android_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) return map_status;
  const auto descriptor_status =
    validate_android_surface_descriptor(descriptor);
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
    "an Android surface session runs on a library thread, since the library "
    "owns its graphics context; attach it with MLN_RENDER_DRIVER_CORE_WORKER"
  );
  if (driver_status != MLN_STATUS_OK) return driver_status;
  auto window = AndroidWindow{};
  const auto window_status = acquire_window(
    descriptor->window, descriptor->jni_env, descriptor->surface, window
  );
  if (window_status != MLN_STATUS_OK) return window_status;

  session->target_identity = {render_target_handle(window.get())};
#if defined(MLN_RENDER_BACKEND_VULKAN)
  session->accepts_queue_lock = true;
#endif
  // The backend takes the window from here, so that this closure, which the
  // session keeps, doesn't hold it past a release of the target.
  session->initialize_backend =
    [attach = AndroidSurfaceAttach{
       .window = std::move(window), .device = std::move(context)
     }](mln_render_session_object& target) mutable {
      return create_android_surface_backend(std::move(attach), target);
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
    valueless_completion<&mln_map_attach_android_surface>()
  );
}

auto android_surface_set_target_start(
  mln_render_session session,
  const mln_android_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  const auto replacement_status =
    validate_android_surface_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) return replacement_status;
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::Surface, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) return submission_status;
  auto window = AndroidWindow{};
  const auto window_status = acquire_window(
    replacement->window, replacement->jni_env, replacement->surface, window
  );
  if (window_status != MLN_STATUS_OK) return window_status;
  auto identity = RenderTargetIdentity{render_target_handle(window.get())};
  return enqueue_surface_retarget(
    session, replacement->logical_size, std::move(identity),
    [window = std::move(window)](
      mln_render_session_object& live, mln::Size physical_size
    ) {
      return live.surface.backend->set_android_target(window, physical_size);
    },
    std::move(checked_completion),
    valueless_completion<&mln_render_session_set_android_surface_target>()
  );
}

}  // namespace mln::core
