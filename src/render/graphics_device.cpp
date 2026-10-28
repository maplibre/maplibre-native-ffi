#include <memory>
#include <utility>

#include "render/graphics_device.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "map/logical_extent.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

namespace {

auto graphics_device_table() -> HandleTable<GraphicsDeviceObject>& {
  return handle_table<GraphicsDeviceObject>();
}

auto effective_graphics_device_options(
  const mln_graphics_device_options* options,
  mln_graphics_device_options& out_options
) -> mln_status {
  out_options = mln_graphics_device_options{
    .size = sizeof(mln_graphics_device_options),
    .fields = 0,
    .adapter_index = 0,
    .single_context = false,
  };
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_graphics_device_options)) {
    set_thread_error("mln_graphics_device_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & ~MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX) != 0U) {
    set_thread_error(
      "mln_graphics_device_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_options.fields = options->fields;
  out_options.adapter_index = options->adapter_index;
  out_options.single_context = options->single_context;
  return MLN_STATUS_OK;
}

auto validate_graphics_device_texture_descriptor(
  const mln_graphics_device_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_graphics_device_texture_descriptor)) {
    set_thread_error(
      "mln_graphics_device_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
}

}  // namespace

#if !defined(MLN_RENDER_BACKEND_METAL) &&  \
  !defined(MLN_RENDER_BACKEND_VULKAN) &&   \
  !defined(MLN_FFI_OPENGL_PROVIDER_EGL) && \
  !defined(MLN_FFI_OPENGL_PROVIDER_WGL)
// A browser host creates its WebGPU device or WebGL context in JavaScript, so
// these builds carry no way to make one.
auto create_backend_graphics_device(
  const mln_graphics_device_options& options,
  std::shared_ptr<GraphicsDevice>& out_device
) -> mln_status {
  (void)options;
  (void)out_device;
  set_thread_error(
    "this build's render backend cannot create a graphics device"
  );
  return MLN_STATUS_UNSUPPORTED;
}
#endif

auto graphics_device_create(
  const mln_graphics_device_options* options, mln_graphics_device* out_device
) -> mln_status {
  auto effective = mln_graphics_device_options{};
  const auto options_status =
    effective_graphics_device_options(options, effective);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  if (out_device == nullptr || *out_device != MLN_HANDLE_NULL) {
    set_thread_error("out_device must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto device = std::shared_ptr<GraphicsDevice>{};
  const auto create_status = create_backend_graphics_device(effective, device);
  if (create_status != MLN_STATUS_OK) {
    return create_status;
  }
  *out_device = graphics_device_table().insert(
    std::make_shared<GraphicsDeviceObject>(
      GraphicsDeviceObject{.device = std::move(device)}
    )
  );
  return MLN_STATUS_OK;
}

auto graphics_device_release(mln_graphics_device device) -> void {
  if (device == MLN_HANDLE_NULL) {
    return;
  }
  graphics_device_table().remove(device);
}

auto graphics_device_texture_attach_start(
  mln_map map, mln_graphics_device device,
  const mln_graphics_device_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto object = graphics_device_table().lease(device);
  if (object == nullptr) {
    return recorded_handle_fault_status();
  }
  const auto descriptor_status =
    validate_graphics_device_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto request_status =
    validate_render_session_attach_request(options, out_session, completion);
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }
  const auto driver_status = require_render_driver(
    options, MLN_RENDER_DRIVER_CORE_WORKER,
    "a graphics device texture renders on a core worker"
  );
  if (driver_status != MLN_STATUS_OK) {
    return driver_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.mode = TextureSessionMode::Owned;
  session->initialize_backend =
    [device = object->device](mln_render_session_object& target) {
      auto backend = std::unique_ptr<TextureSessionBackend>{};
      const auto status = device->create_texture_backend(
        mln::Size{target.physical_width, target.physical_height}, backend
      );
      if (backend != nullptr) {
        backend->retain(device);
        target.texture.backend = std::move(backend);
      }
      return status;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = 1,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_graphics_device_texture>()
  );
}

}  // namespace mln::core
