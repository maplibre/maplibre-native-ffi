// The surface and texture session entry points, and the Vulkan device, for the
// backends this build does not carry.
//
// The validation order is part of what a host sees, so it matches the
// backend-native paths step for step. An attach checks the map, the
// descriptor, then the size, and finally the attachment request. A set_target
// checks its replacement and a borrowed ring's physical size, which a backend's
// set_target also checks before its session. Only once all of those pass does
// the caller hear that the backend is missing.

#include <memory>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/texture_session.hpp"
#include "render/vulkan_device.hpp"

namespace mln::core {
namespace {

auto validate_attach_map(mln_map map) -> mln_status {
  MapObject* live_map = nullptr;
  return validate_map_live(map, live_map);
}

// A surface renders at its map's scale factor.
auto validate_surface_size(mln_map map, const mln_logical_size& size)
  -> mln_status {
  return validate_physical_size(
    size.width, size.height, map_scale_factor(map),
    "scaled target dimensions are too large"
  );
}

auto unsupported(const char* message) -> mln_status {
  set_thread_error(message);
  return MLN_STATUS_UNSUPPORTED;
}

}  // namespace

#if !defined(MLN_RENDER_BACKEND_METAL)
auto metal_surface_attach_start(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_metal_surface_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_surface_size(map, descriptor->logical_size);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Metal surface sessions are not supported by this build");
}
auto metal_borrowed_texture_attach_start(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_metal_borrowed_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      descriptor->physical_width, descriptor->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Metal texture sessions are not supported by this build");
}
auto metal_borrowed_texture_set_target_start(
  mln_render_session, const mln_metal_borrowed_texture_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status =
      validate_metal_borrowed_texture_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      replacement->physical_width, replacement->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Metal texture sessions are not supported by this build");
}
auto metal_surface_set_target_start(
  mln_render_session, const mln_metal_surface_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_metal_surface_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Metal surface sessions are not supported by this build");
}
#endif

#if !defined(MLN_RENDER_BACKEND_VULKAN)
auto create_backend_vulkan_device(
  const VulkanDeviceSelection&,
  std::shared_ptr<const mln_vulkan_context_descriptor>&
) -> mln_status {
  return unsupported("Vulkan devices are not supported by this build");
}
auto vulkan_surface_attach_start(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_vulkan_surface_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_surface_size(map, descriptor->logical_size);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Vulkan surface sessions are not supported by this build");
}
auto vulkan_borrowed_texture_attach_start(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_vulkan_borrowed_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      descriptor->physical_width, descriptor->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Vulkan texture sessions are not supported by this build");
}
auto vulkan_borrowed_texture_set_target_start(
  mln_render_session,
  const mln_vulkan_borrowed_texture_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status =
      validate_vulkan_borrowed_texture_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      replacement->physical_width, replacement->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Vulkan texture sessions are not supported by this build");
}
auto vulkan_surface_set_target_start(
  mln_render_session, const mln_vulkan_surface_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_vulkan_surface_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Vulkan surface sessions are not supported by this build");
}
#endif

#if !defined(MLN_RENDER_BACKEND_OPENGL)
auto opengl_surface_attach_start(
  mln_map map, const mln_open_gl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_opengl_surface_descriptor(descriptor, false);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_surface_size(map, descriptor->logical_size);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("OpenGL surface sessions are not supported by this build");
}
auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_open_gl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status =
      validate_opengl_borrowed_texture_descriptor(descriptor, false);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      descriptor->physical_width, descriptor->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("OpenGL texture sessions are not supported by this build");
}
auto opengl_surface_set_target_start(
  mln_render_session, const mln_open_gl_surface_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_opengl_surface_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("OpenGL surface sessions are not supported by this build");
}
auto opengl_borrowed_texture_set_target_start(
  mln_render_session,
  const mln_open_gl_borrowed_texture_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status =
      validate_opengl_borrowed_texture_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      replacement->physical_width, replacement->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("OpenGL texture sessions are not supported by this build");
}
#endif

#if !defined(MLN_RENDER_BACKEND_WEBGPU)
auto webgpu_borrowed_texture_attach_start(
  mln_map map, const mln_web_gpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_webgpu_borrowed_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      descriptor->physical_width, descriptor->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("WebGPU texture sessions are not supported by this build");
}
auto webgpu_borrowed_texture_set_target_start(
  mln_render_session,
  const mln_web_gpu_borrowed_texture_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status =
      validate_webgpu_borrowed_texture_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_borrowed_physical_size(
      replacement->physical_width, replacement->physical_height
    );
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("WebGPU texture sessions are not supported by this build");
}
auto webgpu_surface_attach_start(
  mln_map map, const mln_web_gpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_webgpu_surface_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_surface_size(map, descriptor->logical_size);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status =
      validate_render_session_attach_request(options, out_session, completion);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("WebGPU surface sessions are not supported by this build");
}
auto webgpu_surface_set_target_start(
  mln_render_session, const mln_web_gpu_surface_replacement* replacement,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_webgpu_surface_replacement(replacement);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("WebGPU surface sessions are not supported by this build");
}
#endif

}  // namespace mln::core
