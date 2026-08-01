// The surface and texture session entry points for the backends this build does
// not carry.
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
#include "render/texture_session.hpp"

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

auto validate_owned_texture_extent(const mln_render_target_extent& extent)
  -> mln_status {
  return validate_physical_size(
    extent.width, extent.height, extent.scale_factor,
    "scaled texture dimensions are too large"
  );
}

// A frame belongs to a session rather than a descriptor, so this checks the
// session handle and the frame's own size, mirroring the backend-native
// acquire and release paths.
template <typename Frame>
auto validate_texture_frame(
  mln_render_session texture, const Frame* frame, const char* message
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame == nullptr || frame->size < sizeof(Frame)) {
    set_thread_error(message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
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

auto metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_metal_owned_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto extent_status = validate_owned_texture_extent(descriptor->extent);
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_metal_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_owned_texture_acquire_frame(
  mln_render_session texture, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_texture_frame(
    texture, out_frame, "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_owned_texture_release_frame(
  mln_render_session texture, const mln_metal_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_texture_frame(
    texture, frame, "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::BorrowedTexture, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status =
    validate_metal_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_surface_set_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::Surface, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status = validate_metal_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
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

auto vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_owned_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto extent_status = validate_owned_texture_extent(descriptor->extent);
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_borrowed_texture_attach(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_acquire_frame(
  mln_render_session texture, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_texture_frame(
    texture, out_frame, "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_release_frame(
  mln_render_session texture, const mln_vulkan_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_texture_frame(
    texture, frame, "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_borrowed_texture_set_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::BorrowedTexture, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status =
    validate_vulkan_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_surface_set_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::Surface, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status = validate_vulkan_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
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

auto opengl_owned_texture_attach(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_owned_texture_descriptor(descriptor, false);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto extent_status = validate_owned_texture_extent(descriptor->extent);
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  set_thread_error("OpenGL texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_borrowed_texture_attach(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  const auto map_status = validate_attach_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_borrowed_texture_descriptor(descriptor, false);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_out_session(out_session);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  set_thread_error("OpenGL texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_owned_texture_acquire_frame(
  mln_render_session texture, mln_opengl_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_texture_frame(
    texture, out_frame, "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  set_thread_error("OpenGL texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_owned_texture_release_frame(
  mln_render_session texture, const mln_opengl_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_texture_frame(
    texture, frame, "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  set_thread_error("OpenGL texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_borrowed_texture_set_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::BorrowedTexture, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status =
    validate_opengl_borrowed_texture_descriptor(descriptor, false);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  set_thread_error("OpenGL texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_surface_set_target(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::Surface, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status =
    validate_opengl_surface_descriptor(descriptor, false);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  set_thread_error("OpenGL surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

#endif

}  // namespace mln::core
