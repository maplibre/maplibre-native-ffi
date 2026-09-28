#define MLN_BUILDING_C

#include <cstddef>
#include <cstdint>

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

auto mln_metal_owned_texture_descriptor_default(void) noexcept
  -> mln_metal_owned_texture_descriptor {
  return mln::core::metal_owned_texture_descriptor_default();
}

auto mln_metal_borrowed_texture_descriptor_default(void) noexcept
  -> mln_metal_borrowed_texture_descriptor {
  return mln::core::metal_borrowed_texture_descriptor_default();
}

auto mln_vulkan_owned_texture_descriptor_default(void) noexcept
  -> mln_vulkan_owned_texture_descriptor {
  return mln::core::vulkan_owned_texture_descriptor_default();
}

auto mln_vulkan_borrowed_texture_descriptor_default(void) noexcept
  -> mln_vulkan_borrowed_texture_descriptor {
  return mln::core::vulkan_borrowed_texture_descriptor_default();
}

auto mln_opengl_owned_texture_descriptor_default(void) noexcept
  -> mln_opengl_owned_texture_descriptor {
  return mln::core::opengl_owned_texture_descriptor_default();
}

auto mln_opengl_borrowed_texture_descriptor_default(void) noexcept
  -> mln_opengl_borrowed_texture_descriptor {
  return mln::core::opengl_borrowed_texture_descriptor_default();
}

auto mln_webgpu_owned_texture_descriptor_default(void) noexcept
  -> mln_webgpu_owned_texture_descriptor {
  return mln::core::webgpu_owned_texture_descriptor_default();
}

auto mln_webgpu_borrowed_texture_descriptor_default(void) noexcept
  -> mln_webgpu_borrowed_texture_descriptor {
  return mln::core::webgpu_borrowed_texture_descriptor_default();
}

auto mln_texture_image_info_default(void) noexcept -> mln_texture_image_info {
  return mln::core::texture_image_info_default();
}

auto mln_supported_render_backend_mask(void) noexcept -> uint32_t {
  return mln::core::supported_render_backend_mask();
}

auto mln_opengl_supported_context_provider_mask(void) noexcept -> uint32_t {
  return mln::core::opengl_supported_context_provider_mask();
}

auto mln_render_target_extent_physical_size(
  const mln_render_target_extent* extent, uint32_t* out_width,
  uint32_t* out_height
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_target_extent_physical_size(
      extent, out_width, out_height
    );
  });
}

auto mln_metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::metal_owned_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::metal_borrowed_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::vulkan_owned_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_vulkan_borrowed_texture_attach(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::vulkan_borrowed_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_opengl_owned_texture_attach(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::opengl_owned_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_opengl_borrowed_texture_attach(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::opengl_borrowed_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_webgpu_owned_texture_attach(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::webgpu_owned_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_webgpu_borrowed_texture_attach(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::webgpu_borrowed_texture_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::metal_borrowed_texture_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_vulkan_borrowed_texture_set_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::vulkan_borrowed_texture_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_opengl_borrowed_texture_set_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::opengl_borrowed_texture_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_webgpu_borrowed_texture_set_target(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::webgpu_borrowed_texture_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_texture_read_premultiplied_rgba8(
  mln_render_session session, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::texture_read_premultiplied_rgba8_start(
      session, completion
    );
  });
}

auto mln_acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_owned_texture_frame* out_frame
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::acquired_frame_get_metal_texture(frame, out_frame);
  });
}

auto mln_acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_owned_texture_frame* out_frame
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::acquired_frame_get_vulkan_texture(frame, out_frame);
  });
}

auto mln_acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_owned_texture_frame* out_frame
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::acquired_frame_get_opengl_texture(frame, out_frame);
  });
}

auto mln_acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_owned_texture_frame* out_frame
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::acquired_frame_get_webgpu_texture(frame, out_frame);
  });
}
