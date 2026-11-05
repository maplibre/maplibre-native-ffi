#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "render/offscreen_device.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "map/logical_extent.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

namespace {

auto offscreen_device_table() -> HandleTable<OffscreenDeviceObject>& {
  return handle_table<OffscreenDeviceObject>();
}

auto default_offscreen_device_options() -> mln_offscreen_device_options {
  return mln_offscreen_device_options{
    .size = sizeof(mln_offscreen_device_options),
    .fields = 0,
    .adapter_index = 0,
    .single_context = false,
    .client_api = MLN_OPEN_GL_CLIENT_API_UNSPECIFIED,
  };
}

auto effective_offscreen_device_options(
  const mln_offscreen_device_options* options,
  mln_offscreen_device_options& out_options
) -> mln_status {
  out_options = default_offscreen_device_options();
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_offscreen_device_options)) {
    set_thread_error("mln_offscreen_device_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & ~(MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX |
                         MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT |
                         MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API)) != 0U
  ) {
    set_thread_error(
      "mln_offscreen_device_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_options.fields = options->fields;
  out_options.adapter_index = options->adapter_index;
  out_options.single_context =
    (options->fields & MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT) != 0U &&
    options->single_context;
  if ((options->fields & MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API) != 0U) {
    if (
      options->client_api != MLN_OPEN_GL_CLIENT_API_GL &&
      options->client_api != MLN_OPEN_GL_CLIENT_API_GLES
    ) {
      set_thread_error(
        "mln_offscreen_device_options.client_api must name an OpenGL API"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
#if defined(MLN_RENDER_BACKEND_OPENGL)
    out_options.client_api = options->client_api;
#else
    set_thread_error(
      "this build's render backend renders with no OpenGL API, so it takes no "
      "client_api"
    );
    return MLN_STATUS_UNSUPPORTED;
#endif
  }
  return MLN_STATUS_OK;
}

auto validate_offscreen_descriptor(const mln_offscreen_descriptor* descriptor)
  -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_offscreen_descriptor)) {
    set_thread_error("mln_offscreen_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

// Makes the session's backend on `device`, which the backend then shares in
// owning.
auto initialize_offscreen_backend(
  mln_render_session_object& target,
  const std::shared_ptr<OffscreenDevice>& device
) -> mln_status {
  auto backend = std::unique_ptr<TextureSessionBackend>{};
  const auto status = device->create_texture_backend(
    mln::Size{target.physical_width, target.physical_height}, backend
  );
  if (backend != nullptr) {
    backend->retain(device);
    target.texture.backend = std::move(backend);
  }
  return status;
}

#if defined(MLN_RENDER_BACKEND_WEBGPU)
// A browser's WebGPU device belongs to the worker that requested it, and the
// browser answers the request on that worker's event loop. So the session's
// core worker runs from its event loop and requests its device before it takes
// any work, and no caller can create one to share.
auto configure_webgpu_offscreen_worker(mln_render_session_object& session)
  -> void {
  struct Requested {
    std::shared_ptr<OffscreenDevice> device;
    std::string failure = "requesting a WebGPU device failed";
  };
  // Only the core worker touches it, first in the request and then in the
  // attach.
  auto requested = std::make_shared<Requested>();
  session.core_worker_yields_to_event_loop = true;
  session.prepare_core_worker = [requested](std::function<void()> start) {
    auto done =
      std::function<void(std::shared_ptr<OffscreenDevice>, std::string)>{};
    try {
      done = [requested, start](
               std::shared_ptr<OffscreenDevice> device, std::string failure
             ) {
        requested->device = std::move(device);
        if (!failure.empty()) requested->failure = std::move(failure);
        start();
      };
    } catch (...) {
      // The attach then reports the default failure.
      start();
      return;
    }
    request_backend_offscreen_device(std::move(done));
  };
  session.initialize_backend = [requested](mln_render_session_object& target) {
    // Taken, so the backend is its only owner: the worker exits only after
    // the backend destroys it, and the session outlives the worker.
    const auto device = std::exchange(requested->device, nullptr);
    if (device == nullptr) {
      set_thread_error(requested->failure.c_str());
      return MLN_STATUS_NATIVE_ERROR;
    }
    return initialize_offscreen_backend(target, device);
  };
}
#endif

}  // namespace

auto offscreen_device_create(
  const mln_offscreen_device_options* options, mln_offscreen_device* out_device
) -> mln_status {
  auto effective = mln_offscreen_device_options{};
  const auto options_status =
    effective_offscreen_device_options(options, effective);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  if (out_device == nullptr || *out_device != MLN_HANDLE_NULL) {
    set_thread_error("out_device must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
#if defined(MLN_RENDER_BACKEND_WEBGPU)
  set_thread_error(
    "this build cannot create an offscreen device; attach without one, "
    "and the session creates its own"
  );
  return MLN_STATUS_UNSUPPORTED;
#else
  auto device = std::shared_ptr<OffscreenDevice>{};
  const auto create_status = create_backend_offscreen_device(effective, device);
  if (create_status != MLN_STATUS_OK) {
    return create_status;
  }
  *out_device = offscreen_device_table().insert(
    std::make_shared<OffscreenDeviceObject>(
      OffscreenDeviceObject{.device = std::move(device)}
    )
  );
  return MLN_STATUS_OK;
#endif
}

auto offscreen_device_release(mln_offscreen_device device) -> void {
  if (device == MLN_HANDLE_NULL) {
    return;
  }
  offscreen_device_table().remove(device);
}

auto offscreen_attach_start(
  mln_map map, const mln_offscreen_descriptor* descriptor,
  const mln_render_session_attach_options* options, mln_offscreen_device device,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  auto object = std::shared_ptr<OffscreenDeviceObject>{};
  if (device != MLN_HANDLE_NULL) {
    object = offscreen_device_table().lease(device);
    if (object == nullptr) {
      return recorded_handle_fault_status();
    }
  }
  const auto descriptor_status = validate_offscreen_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  const auto size_status = size_new_session(*session, descriptor->logical_size);
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  const auto request_status =
    validate_render_session_attach_request(options, out_session, completion);
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }
  const auto driver_status = require_render_driver(
    options, MLN_RENDER_DRIVER_CORE_WORKER,
    "an offscreen session renders on a core worker"
  );
  if (driver_status != MLN_STATUS_OK) {
    return driver_status;
  }
  session->texture.mode = TextureSessionMode::Offscreen;
#if defined(MLN_RENDER_BACKEND_WEBGPU)
  // No device handle exists to share; see offscreen_device_create().
  configure_webgpu_offscreen_worker(*session);
#else
  session->initialize_backend =
    [shared =
       object != nullptr ? object->device : std::shared_ptr<OffscreenDevice>{}](
      mln_render_session_object& target
    ) {
      // Without a shared device, the session's backend is the only owner of
      // the device it creates here.
      auto device = shared;
      if (device == nullptr) {
        const auto create_status = create_backend_offscreen_device(
          default_offscreen_device_options(), device
        );
        if (create_status != MLN_STATUS_OK) {
          return create_status;
        }
      }
      return initialize_offscreen_backend(target, device);
    };
#endif
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = 1,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK,
    .producer_sync_kind = {},
    .consumer_sync_kinds = 0,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion, valueless_completion<&mln_map_attach_offscreen>()
  );
}

}  // namespace mln::core
