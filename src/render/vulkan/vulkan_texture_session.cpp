#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <mln/gfx/headless_backend.hpp>
#include <mln/util/size.hpp>

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
    const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth
  )
      : backend_(descriptor, size, ring_depth) {}

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

  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }

  auto copy_slot_metadata(
    const mln_render_session_object& texture, std::size_t slot,
    std::any& out_metadata
  ) -> mln_status override {
    const auto resources = backend_.frame_resources(slot);
    out_metadata = mln_vulkan_owned_texture_frame{
      .size = sizeof(mln_vulkan_owned_texture_frame),
      .generation = texture.generation,
      .width = texture.physical_width,
      .height = texture.physical_height,
      .scale_factor = texture.scale_factor,
      .frame_id = texture.frame_generation,
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

auto vulkan_owned_texture_attach_start(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
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
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::Vulkan;
  session->texture.mode = TextureSessionMode::Owned;
  const auto copied = *descriptor;
  const auto ring_depth = std::clamp(
    options == nullptr ? 1u : options->requested_texture_ring_depth, 1u, 3u
  );
  session->initialize_backend =
    [copied, ring_depth](mln_render_session_object& target) {
      const auto handles_status = validate_vulkan_handles(copied);
      if (handles_status != MLN_STATUS_OK) {
        return handles_status;
      }
      target.texture.backend = std::make_unique<VulkanTextureSessionBackend>(
        copied, mln::Size{target.physical_width, target.physical_height},
        ring_depth
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = ring_depth,
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
             MLN_RENDER_SESSION_CAPABILITY_READBACK |
             MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion
  );
}

auto vulkan_borrowed_texture_attach_start(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
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
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_borrowed_session_extent(
    *session, descriptor->extent, descriptor->physical_width,
    descriptor->physical_height
  );
  session->texture.api_kind = TextureSessionApi::Vulkan;
  session->texture.mode = TextureSessionMode::Borrowed;
  const auto copied = *descriptor;
  session->initialize_backend = [copied](mln_render_session_object& target) {
    const auto handles = mln_vulkan_owned_texture_descriptor{
      .size = sizeof(mln_vulkan_owned_texture_descriptor),
      .extent = copied.extent,
      .context = copied.context,
    };
    const auto handles_status = validate_vulkan_handles(handles);
    if (handles_status != MLN_STATUS_OK) {
      return handles_status;
    }
    target.texture.backend = std::make_unique<VulkanTextureSessionBackend>(
      copied, mln::Size{target.physical_width, target.physical_height}
    );
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion
  );
}

auto vulkan_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status {
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
  const auto copied = *descriptor;
  return enqueue_driver_operation(
    session,
    [copied](mln_render_session_object& target) {
      return render_session_set_target(
        target.self, RetargetTargetKind::BorrowedTexture, copied.extent,
        copied.physical_width, copied.physical_height,
        [&copied](mln_render_session_object& live) {
          return live.texture.backend->set_vulkan_borrowed_target(copied);
        }
      );
    },
    completion
  );
}

}  // namespace mln::core
