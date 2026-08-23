#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <mbgl/gfx/headless_backend.hpp>
#include <mbgl/util/size.hpp>

#include <vulkan/vulkan_core.h>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/render_target.h"
#include "maplibre_native_c/texture.h"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"
#include "render/vulkan/vulkan_dispatch.hpp"
#include "render/vulkan/vulkan_handle.hpp"
#include "render/vulkan/vulkan_texture_backend.hpp"

namespace {

// The shared descriptor defaults and validator are built without the Vulkan
// headers, so they spell these as plain zeros.
static_assert(VK_FORMAT_UNDEFINED == 0);
static_assert(VK_IMAGE_LAYOUT_UNDEFINED == 0);

auto validate_vulkan_handles(
  const mln_vulkan_owned_texture_descriptor& descriptor
) -> mln_status {
  auto* const instance = static_cast<VkInstance>(descriptor.context.instance);
  auto* const physical_device =
    static_cast<VkPhysicalDevice>(descriptor.context.physical_device);

  auto dispatcher = mln::core::vulkan_dispatch_loader(descriptor.context);
  mln::core::vulkan_init_instance_dispatch(dispatcher, descriptor.context);
  if (
    dispatcher.vkEnumeratePhysicalDevices == nullptr ||
    dispatcher.vkGetPhysicalDeviceQueueFamilyProperties == nullptr
  ) {
    mln::core::set_thread_error("Vulkan dispatch functions must resolve");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto physical_device_count = uint32_t{};
  auto result = dispatcher.vkEnumeratePhysicalDevices(
    instance, &physical_device_count, nullptr
  );
  if (result != VK_SUCCESS || physical_device_count == 0) {
    mln::core::set_thread_error(
      "Vulkan instance must expose at least one physical device"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto physical_devices = std::vector<VkPhysicalDevice>(physical_device_count);
  result = dispatcher.vkEnumeratePhysicalDevices(
    instance, &physical_device_count, physical_devices.data()
  );
  if (result != VK_SUCCESS) {
    mln::core::set_thread_error("failed to enumerate Vulkan physical devices");
    return MLN_STATUS_NATIVE_ERROR;
  }

  auto found_physical_device = false;
  for (auto* const candidate : physical_devices) {
    if (candidate == physical_device) {
      found_physical_device = true;
      break;
    }
  }
  if (!found_physical_device) {
    mln::core::set_thread_error(
      "Vulkan physical_device must belong to instance"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto queue_family_count = uint32_t{};
  dispatcher.vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device, &queue_family_count, nullptr
  );
  if (descriptor.context.graphics_queue_family_index >= queue_family_count) {
    mln::core::set_thread_error(
      "Vulkan graphics_queue_family_index is out of range"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto queue_families =
    std::vector<VkQueueFamilyProperties>(queue_family_count);
  dispatcher.vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device, &queue_family_count, queue_families.data()
  );
  const auto& queue_family =
    queue_families.at(descriptor.context.graphics_queue_family_index);
  if (
    (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ||
    queue_family.queueCount == 0
  ) {
    mln::core::set_thread_error(
      "Vulkan graphics_queue_family_index must support graphics"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

class VulkanTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  VulkanTextureSessionBackend(
    const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size
  )
      : backend_(descriptor, size) {}

  VulkanTextureSessionBackend(
    const mln_vulkan_borrowed_texture_descriptor& descriptor, mln::Size size
  )
      : backend_(descriptor, size) {}

  auto headless_backend() -> mln::gfx::HeadlessBackend& override {
    return backend_;
  }

  void resize(mln::Size size) override { backend_.resize(size); }

  auto set_vulkan_borrowed_target(
    const mln_vulkan_borrowed_texture_descriptor& descriptor
  ) -> mln_status override {
    if (!mln::core::vulkan_context_matches(
          backend_.context_descriptor(), descriptor.context
        )) {
      mln::core::set_thread_error(
        "Vulkan texture target must name the instance, physical device, "
        "device, and graphics queue this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!backend_.matches_borrowed_target(descriptor)) {
      return mln::core::unsupported_retarget(
        "Vulkan image target must have the format and layouts this session's "
        "render pass was built for; destroy the session and attach again to "
        "change them"
      );
    }
    backend_.set_borrowed_target(descriptor);
    return MLN_STATUS_OK;
  }

  void prepare_render_resources() override {
    // Renderer::render creates the Vulkan context before requesting the default
    // renderable, so shared-device resources must be ready first.
    backend_.prepareRenderResources();
  }

  auto acquire_vulkan_owned_frame(
    const mln_render_session_object& texture,
    mln_vulkan_owned_texture_frame& out_frame
  ) -> mln_status override {
    const auto resources = backend_.frame_resources();
    out_frame = mln_vulkan_owned_texture_frame{
      .size = sizeof(mln_vulkan_owned_texture_frame),
      .generation = texture.generation,
      .width = texture.physical_width,
      .height = texture.physical_height,
      .scale_factor = texture.scale_factor,
      .frame_id = texture.texture.next_frame_id,
      .image = mln::core::vulkan_handle_to_abi(resources.image),
      .image_view = mln::core::vulkan_handle_to_abi(resources.image_view),
      .device = resources.device,
      .format = static_cast<uint32_t>(resources.format),
      .layout = static_cast<uint32_t>(vk::ImageLayout::eShaderReadOnlyOptimal),
    };
    return MLN_STATUS_OK;
  }

 private:
  mln::core::VulkanTextureBackend backend_;
};

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_VULKAN;
}

auto vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_owned_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto vulkan_status = validate_vulkan_handles(*descriptor);
  if (vulkan_status != MLN_STATUS_OK) {
    return vulkan_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::Vulkan;
  session->texture.mode = TextureSessionMode::Owned;
  session->texture.backend = std::make_unique<VulkanTextureSessionBackend>(
    *descriptor, mln::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Texture,
    RenderSessionAttachMessages{
      .null_session = "texture session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto vulkan_borrowed_texture_attach(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  auto handle_descriptor = mln_vulkan_owned_texture_descriptor{
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    .extent = descriptor->extent,
    .context = descriptor->context,
  };
  const auto vulkan_status = validate_vulkan_handles(handle_descriptor);
  if (vulkan_status != MLN_STATUS_OK) {
    return vulkan_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_borrowed_session_extent(
    *session, descriptor->extent, descriptor->physical_width,
    descriptor->physical_height
  );
  session->texture.api_kind = TextureSessionApi::Vulkan;
  session->texture.mode = TextureSessionMode::Borrowed;
  session->texture.backend = std::make_unique<VulkanTextureSessionBackend>(
    *descriptor, mln::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Texture,
    RenderSessionAttachMessages{
      .null_session = "texture session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto vulkan_owned_texture_acquire_frame(
  mln_render_session texture, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_frame == nullptr ||
    out_frame->size < sizeof(mln_vulkan_owned_texture_frame)
  ) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (live->texture.acquired) {
    set_thread_error("a texture frame is already acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->rendered_generation != live->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    live->texture.mode != TextureSessionMode::Owned ||
    live->texture.api_kind != TextureSessionApi::Vulkan
  ) {
    set_thread_error("texture session cannot expose a Vulkan texture frame");
    return MLN_STATUS_UNSUPPORTED;
  }

  const auto acquire_status =
    live->texture.backend->acquire_vulkan_owned_frame(*live, *out_frame);
  if (acquire_status != MLN_STATUS_OK) {
    return acquire_status;
  }
  live->texture.acquired = true;
  live->texture.acquired_frame_id = out_frame->frame_id;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::VulkanOwned;
  ++live->texture.next_frame_id;
  return MLN_STATUS_OK;
}

auto vulkan_owned_texture_release_frame(
  mln_render_session texture, const mln_vulkan_owned_texture_frame* frame
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    frame == nullptr || frame->size < sizeof(mln_vulkan_owned_texture_frame)
  ) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !live->texture.acquired ||
    live->texture.acquired_frame_kind != TextureSessionFrameKind::VulkanOwned
  ) {
    set_thread_error("no texture frame is currently acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (frame->generation != live->generation) {
    set_thread_error("frame generation does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (frame->frame_id != live->texture.acquired_frame_id) {
    set_thread_error("frame identity does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  live->texture.acquired = false;
  live->texture.acquired_frame_id = 0;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  return MLN_STATUS_OK;
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
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  return render_session_set_target(
    session, RetargetTargetKind::BorrowedTexture, descriptor->extent,
    descriptor->physical_width, descriptor->physical_height,
    [descriptor](mln_render_session_object& target_session) -> mln_status {
      return target_session.texture.backend->set_vulkan_borrowed_target(
        *descriptor
      );
    }
  );
}

}  // namespace mln::core
