// The surface and texture session entry points for the backends this build does
// not carry.
//
// The validation order is part of what a host sees, so it matches the
// backend-native attach paths step for step: the map, the descriptor, then the
// extent, and finally the attachment request. Only once all of those pass does
// the caller hear that the backend is missing.

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/texture_session.hpp"

namespace mln::core {
namespace {

auto validate_attach_map(mln_map map) -> mln_status {
  MapObject* live_map = nullptr;
  return validate_map_live(map, live_map);
}

auto validate_surface_extent(const mln_render_target_extent& extent)
  -> mln_status {
  return validate_physical_size(
    extent.width, extent.height, extent.scale_factor,
    "scaled surface dimensions are too large"
  );
}

auto validate_owned_texture_extent(const mln_render_target_extent& extent)
  -> mln_status {
  return validate_physical_size(
    extent.width, extent.height, extent.scale_factor,
    "scaled texture dimensions are too large"
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
    const auto status = validate_surface_extent(descriptor->extent);
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
auto metal_owned_texture_attach_start(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_metal_owned_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_owned_texture_extent(descriptor->extent);
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
  mln_render_session, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_metal_borrowed_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Metal texture sessions are not supported by this build");
}
auto metal_surface_set_target_start(
  mln_render_session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_metal_surface_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Metal surface sessions are not supported by this build");
}
#endif

#if !defined(MLN_RENDER_BACKEND_VULKAN)
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
    const auto status = validate_surface_extent(descriptor->extent);
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
auto vulkan_owned_texture_attach_start(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_vulkan_owned_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_owned_texture_extent(descriptor->extent);
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
  mln_render_session, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_vulkan_borrowed_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Vulkan texture sessions are not supported by this build");
}
auto vulkan_surface_set_target_start(
  mln_render_session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_vulkan_surface_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("Vulkan surface sessions are not supported by this build");
}
#endif

#if !defined(MLN_RENDER_BACKEND_OPENGL)
auto opengl_surface_attach_start(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
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
    const auto status = validate_surface_extent(descriptor->extent);
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
auto opengl_owned_texture_attach_start(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status =
      validate_opengl_owned_texture_descriptor(descriptor, false);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_owned_texture_extent(descriptor->extent);
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
auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
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
  mln_render_session, const mln_opengl_surface_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_opengl_surface_descriptor(descriptor, false);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("OpenGL surface sessions are not supported by this build");
}
auto opengl_borrowed_texture_set_target_start(
  mln_render_session, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status =
      validate_opengl_borrowed_texture_descriptor(descriptor, false);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("OpenGL texture sessions are not supported by this build");
}
#endif

#if !defined(MLN_RENDER_BACKEND_WEBGPU)
auto webgpu_owned_texture_attach_start(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  if (const auto status = validate_attach_map(map); status != MLN_STATUS_OK)
    return status;
  if (
    const auto status = validate_webgpu_owned_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  if (
    const auto status = validate_owned_texture_extent(descriptor->extent);
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
auto webgpu_borrowed_texture_attach_start(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
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
  mln_render_session, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_webgpu_borrowed_texture_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("WebGPU texture sessions are not supported by this build");
}
auto webgpu_surface_attach_start(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
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
    const auto status = validate_surface_extent(descriptor->extent);
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
  mln_render_session, const mln_webgpu_surface_descriptor* descriptor,
  const mln_completion*
) -> mln_status {
  if (
    const auto status = validate_webgpu_surface_descriptor(descriptor);
    status != MLN_STATUS_OK
  )
    return status;
  return unsupported("WebGPU surface sessions are not supported by this build");
}
#endif

}  // namespace mln::core
