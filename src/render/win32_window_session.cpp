#include <memory>
#include <utility>

#include "render/win32_window_session.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/vulkan_device.hpp"
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
#include "render/opengl/wgl_window.hpp"
#endif

namespace mln::core {
namespace {

auto borrow_window(void* window) -> Win32Window {
  return Win32Window{static_cast<HWND>(window), [](HWND) {}};
}

// A minimized window's surface has no extent, which no swapchain can have. The
// window can be minimized between the call and its work on the driver, so the
// driver checks again.
auto require_ready_window(HWND window) -> mln_status {
  if (!win32_window_ready(window)) {
    set_thread_error("the Win32 window is minimized or has no client area");
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto validate_window(void* window) -> mln_status {
  auto* const hwnd = static_cast<HWND>(window);
  if (IsWindow(hwnd) == FALSE) {
    set_thread_error("the Win32 window is not a window");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto ready = require_ready_window(hwnd);
  if (ready != MLN_STATUS_OK) return ready;
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  return opengl::validate_wgl_window_pixel_format(hwnd);
#else
  return MLN_STATUS_OK;
#endif
}

}  // namespace

auto win32_window_ready(HWND window) -> bool {
  if (IsIconic(window) != FALSE) return false;
  auto client = RECT{};
  return GetClientRect(window, &client) != FALSE && client.right > 0 &&
         client.bottom > 0;
}

auto win32_window_attach_start(
  mln_map map, const mln_win32_window_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) return map_status;
  const auto descriptor_status = validate_win32_window_descriptor(descriptor);
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
    "a Win32 window session runs on a library thread, since the library owns "
    "its graphics context; attach it with MLN_RENDER_DRIVER_CORE_WORKER"
  );
  if (driver_status != MLN_STATUS_OK) return driver_status;
  const auto window_status = validate_window(descriptor->window);
  if (window_status != MLN_STATUS_OK) return window_status;

  session->target_identity = {render_target_handle(descriptor->window)};
#if defined(MLN_RENDER_BACKEND_VULKAN)
  session->accepts_queue_lock = true;
#endif
  // The backend takes the window from here, so that this closure, which the
  // session keeps, doesn't name it past a release of the target.
  session->initialize_backend = [attach = Win32WindowAttach{
                                   .window = borrow_window(descriptor->window),
                                   .device = std::move(context),
                                 }](mln_render_session_object& target) mutable {
    const auto ready = require_ready_window(attach.window.get());
    if (ready != MLN_STATUS_OK) return ready;
    return create_win32_window_backend(std::move(attach), target);
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
    valueless_completion<&mln_map_attach_win32_window>()
  );
}

auto win32_window_set_target_start(
  mln_render_session session, const mln_win32_window_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  const auto replacement_status =
    validate_win32_window_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) return replacement_status;
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::Surface, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) return submission_status;
  const auto window_status = validate_window(replacement->window);
  if (window_status != MLN_STATUS_OK) return window_status;
  return enqueue_surface_retarget(
    session, replacement->logical_size,
    {render_target_handle(replacement->window)},
    [window = borrow_window(replacement->window)](
      mln_render_session_object& live, mln::Size physical_size
    ) {
      const auto ready = require_ready_window(window.get());
      if (ready != MLN_STATUS_OK) return ready;
      return live.surface.backend->set_win32_target(window, physical_size);
    },
    std::move(checked_completion),
    valueless_completion<&mln_render_session_set_win32_window_target>()
  );
}

}  // namespace mln::core
