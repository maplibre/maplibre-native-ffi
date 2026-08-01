// The surface session entry points for the backends this build does not carry.
//
// Every backend's session file used to define the other backends' stubs as
// well, so each stub existed once per backend and the copies drifted: the
// OpenGL build's Metal and Vulkan stubs had fallen a null check and an overflow
// check behind, which made the same C call report a different status depending
// on which backend the library was built with. One definition per entry point,
// guarded by the backend the build selected, keeps them in step, and adding a
// backend means adding its own session file plus one guard here rather than a
// stub in every other backend's file.
//
// The order these validate in is part of what a host sees, so it matches the
// backend-native attach paths step for step: the map, then the descriptor, then
// the output handle, then the scaled extent. Only once all of those pass does
// the caller hear that the backend is missing.

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"

namespace mln::core {
namespace {

auto validate_attach_map(mln_map map) -> mln_status {
  MapObject* live_map = nullptr;
  return validate_map_live(map, live_map);
}

auto validate_attach_out_session(mln_render_session* out_session)
  -> mln_status {
  return validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
}

auto validate_surface_extent(const mln_render_target_extent& extent)
  -> mln_status {
  return validate_physical_size(
    extent.width, extent.height, extent.scale_factor,
    "scaled surface dimensions are too large"
  );
}

}  // namespace

#if !defined(MLN_RENDER_BACKEND_METAL)

auto metal_surface_attach(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto extent_status = validate_surface_extent(descriptor->extent);
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  set_thread_error("Metal surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

#endif

#if !defined(MLN_RENDER_BACKEND_VULKAN)

auto vulkan_surface_attach(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_vulkan_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto extent_status = validate_surface_extent(descriptor->extent);
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  set_thread_error("Vulkan surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

#endif

#if !defined(MLN_RENDER_BACKEND_OPENGL)

auto opengl_surface_attach(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_surface_descriptor(descriptor, false);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto extent_status = validate_surface_extent(descriptor->extent);
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  set_thread_error("OpenGL surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

#endif

}  // namespace mln::core
