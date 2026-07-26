#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/texture_session.hpp"

namespace {

auto unsupported(const char* message) -> mln_status {
  mln::core::set_thread_error(message);
  return MLN_STATUS_UNSUPPORTED;
}

template <typename Descriptor>
auto validate_descriptor_header(
  const Descriptor* descriptor, const char* null_message,
  const char* size_message, const char* dimension_message
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error(null_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(Descriptor)) {
    mln::core::set_thread_error(size_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return mln::core::validate_render_target_extent(
    descriptor->extent, dimension_message
  );
}

auto validate_descriptor(const mln_metal_surface_descriptor* descriptor)
  -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "surface descriptor must not be null",
    "mln_metal_surface_descriptor.size is too small",
    "surface dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  const auto context_status =
    mln::core::validate_metal_context(descriptor->context, false);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->layer == nullptr) {
    mln::core::set_thread_error("Metal surface layer must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_descriptor(const mln_vulkan_surface_descriptor* descriptor)
  -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "surface descriptor must not be null",
    "mln_vulkan_surface_descriptor.size is too small",
    "surface dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  const auto context_status = mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan surface handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == nullptr) {
    mln::core::set_thread_error("Vulkan surface handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_descriptor(const mln_opengl_surface_descriptor* descriptor)
  -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "surface descriptor must not be null",
    "mln_opengl_surface_descriptor.size is too small",
    "surface dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  const auto context_status =
    mln::core::validate_opengl_context(descriptor->context, false);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == nullptr) {
    mln::core::set_thread_error("OpenGL surface must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_descriptor(const mln_metal_owned_texture_descriptor* descriptor)
  -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "texture descriptor must not be null",
    "mln_metal_owned_texture_descriptor.size is too small",
    "texture dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  return mln::core::validate_metal_context(descriptor->context, true);
}

auto validate_descriptor(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "texture descriptor must not be null",
    "mln_metal_borrowed_texture_descriptor.size is too small",
    "texture dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  if (descriptor->texture == nullptr) {
    mln::core::set_thread_error("Metal texture must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_descriptor(const mln_vulkan_owned_texture_descriptor* descriptor)
  -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "texture descriptor must not be null",
    "mln_vulkan_owned_texture_descriptor.size is too small",
    "texture dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  return mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
}

auto validate_descriptor(
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "texture descriptor must not be null",
    "mln_vulkan_borrowed_texture_descriptor.size is too small",
    "texture dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  const auto context_status = mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->image == nullptr || descriptor->image_view == nullptr) {
    mln::core::set_thread_error("Vulkan handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->format == 0 || descriptor->final_layout == 0) {
    mln::core::set_thread_error(
      "Vulkan format and final_layout must be specified"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_descriptor(const mln_opengl_owned_texture_descriptor* descriptor)
  -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "texture descriptor must not be null",
    "mln_opengl_owned_texture_descriptor.size is too small",
    "texture dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  return mln::core::validate_opengl_context(descriptor->context, false);
}

auto validate_descriptor(
  const mln_opengl_borrowed_texture_descriptor* descriptor
) -> mln_status {
  const auto header_status = validate_descriptor_header(
    descriptor, "texture descriptor must not be null",
    "mln_opengl_borrowed_texture_descriptor.size is too small",
    "texture dimensions and scale_factor must be positive"
  );
  if (header_status != MLN_STATUS_OK) {
    return header_status;
  }
  const auto context_status =
    mln::core::validate_opengl_context(descriptor->context, false);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->texture == 0 || descriptor->target == 0) {
    mln::core::set_thread_error("OpenGL texture and target must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto texture_2d = uint32_t{0x0de1};
  if (descriptor->target != texture_2d) {
    mln::core::set_thread_error("OpenGL texture target must be GL_TEXTURE_2D");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

template <typename Descriptor>
auto validate_attach_common(
  mln_map* map, const Descriptor* descriptor, mln_render_session** out_session
) -> mln_status {
  const auto map_status = mln::core::validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = mln::core::validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  return MLN_STATUS_OK;
}

template <typename Descriptor>
auto validate_owned_attach(
  mln_map* map, const Descriptor* descriptor, mln_render_session** out_session
) -> mln_status {
  const auto common_status =
    validate_attach_common(map, descriptor, out_session);
  if (common_status != MLN_STATUS_OK) {
    return common_status;
  }
  return mln::core::validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
}

template <typename Descriptor>
auto validate_borrowed_attach(
  mln_map* map, const Descriptor* descriptor, mln_render_session** out_session
) -> mln_status {
  const auto common_status =
    validate_attach_common(map, descriptor, out_session);
  if (common_status != MLN_STATUS_OK) {
    return common_status;
  }
  return mln::core::validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
}

template <typename Descriptor>
auto validate_surface_attach(
  mln_map* map, const Descriptor* descriptor, mln_render_session** out_session
) -> mln_status {
  const auto common_status =
    validate_attach_common(map, descriptor, out_session);
  if (common_status != MLN_STATUS_OK) {
    return common_status;
  }
  return mln::core::validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
}

template <typename Frame>
auto validate_frame(mln_render_session* session, const Frame* frame)
  -> mln_status {
  const auto session_status = mln::core::validate_texture(session);
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  if (frame == nullptr || frame->size < sizeof(Frame)) {
    mln::core::set_thread_error(
      "frame must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace

namespace mln::core {

auto metal_owned_texture_descriptor_default() noexcept
  -> mln_metal_owned_texture_descriptor {
  return mln_metal_owned_texture_descriptor{
    .size = sizeof(mln_metal_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_metal_context_descriptor{
      .size = sizeof(mln_metal_context_descriptor),
      .device = nullptr,
    },
  };
}

auto metal_borrowed_texture_descriptor_default() noexcept
  -> mln_metal_borrowed_texture_descriptor {
  return mln_metal_borrowed_texture_descriptor{
    .size = sizeof(mln_metal_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .texture = nullptr,
  };
}

auto vulkan_owned_texture_descriptor_default() noexcept
  -> mln_vulkan_owned_texture_descriptor {
  return mln_vulkan_owned_texture_descriptor{
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_vulkan_context_descriptor{
      .size = sizeof(mln_vulkan_context_descriptor),
      .instance = nullptr,
      .physical_device = nullptr,
      .device = nullptr,
      .graphics_queue = nullptr,
      .graphics_queue_family_index = 0,
      .get_instance_proc_addr = nullptr,
      .get_device_proc_addr = nullptr,
    },
  };
}

auto vulkan_borrowed_texture_descriptor_default() noexcept
  -> mln_vulkan_borrowed_texture_descriptor {
  return mln_vulkan_borrowed_texture_descriptor{
    .size = sizeof(mln_vulkan_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context =
      mln_vulkan_context_descriptor{
        .size = sizeof(mln_vulkan_context_descriptor),
        .instance = nullptr,
        .physical_device = nullptr,
        .device = nullptr,
        .graphics_queue = nullptr,
        .graphics_queue_family_index = 0,
        .get_instance_proc_addr = nullptr,
        .get_device_proc_addr = nullptr,
      },
    .image = nullptr,
    .image_view = nullptr,
    .format = 0,
    .initial_layout = 0,
    .final_layout = 5,
  };
}

auto metal_surface_attach(
  mln_map* map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_surface_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Metal surface sessions are not supported by this build");
}

auto vulkan_surface_attach(
  mln_map* map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_surface_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Vulkan surface sessions are not supported by this build");
}

auto opengl_surface_attach(
  mln_map* map, const mln_opengl_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_surface_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("OpenGL surface sessions are not supported by this build");
}

auto metal_owned_texture_attach(
  mln_map* map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_owned_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Metal texture sessions are not supported by this build");
}

auto metal_borrowed_texture_attach(
  mln_map* map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_borrowed_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Metal texture sessions are not supported by this build");
}

auto vulkan_owned_texture_attach(
  mln_map* map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_owned_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Vulkan texture sessions are not supported by this build");
}

auto vulkan_borrowed_texture_attach(
  mln_map* map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_borrowed_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Vulkan texture sessions are not supported by this build");
}

auto opengl_owned_texture_attach(
  mln_map* map, const mln_opengl_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_owned_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("OpenGL texture sessions are not supported by this build");
}

auto opengl_borrowed_texture_attach(
  mln_map* map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto status = validate_borrowed_attach(map, descriptor, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("OpenGL texture sessions are not supported by this build");
}

auto metal_owned_texture_acquire_frame(
  mln_render_session* session, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(session, out_frame);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Metal texture sessions are not supported by this build");
}

auto metal_owned_texture_release_frame(
  mln_render_session* session, const mln_metal_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(session, frame);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Metal texture sessions are not supported by this build");
}

auto vulkan_owned_texture_acquire_frame(
  mln_render_session* session, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(session, out_frame);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Vulkan texture sessions are not supported by this build");
}

auto vulkan_owned_texture_release_frame(
  mln_render_session* session, const mln_vulkan_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(session, frame);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("Vulkan texture sessions are not supported by this build");
}

auto opengl_owned_texture_acquire_frame(
  mln_render_session* session, mln_opengl_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(session, out_frame);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("OpenGL texture sessions are not supported by this build");
}

auto opengl_owned_texture_release_frame(
  mln_render_session* session, const mln_opengl_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(session, frame);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return unsupported("OpenGL texture sessions are not supported by this build");
}

}  // namespace mln::core
