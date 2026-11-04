#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "render/vulkan_device.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

namespace {

auto vulkan_device_table() -> HandleTable<VulkanDeviceObject>& {
  return handle_table<VulkanDeviceObject>();
}

// Copies a GPU ID of `Size` bytes, or reports which member is malformed.
template <std::size_t Size>
auto copy_device_id(
  const mln_buffer_view& view, const char* message,
  std::optional<std::array<std::uint8_t, Size>>& out_id
) -> mln_status {
  if (view.size != Size || view.data == nullptr) {
    set_thread_error(message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto id = std::array<std::uint8_t, Size>{};
  const auto* const bytes = static_cast<const std::uint8_t*>(view.data);
  std::copy_n(bytes, Size, id.begin());
  out_id = id;
  return MLN_STATUS_OK;
}

auto vulkan_device_selection(
  const mln_vulkan_device_options* options, VulkanDeviceSelection& out_selection
) -> mln_status {
  out_selection = VulkanDeviceSelection{};
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_vulkan_device_options)) {
    set_thread_error("mln_vulkan_device_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & ~(MLN_VULKAN_DEVICE_OPTION_PRESENTATION |
                         MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID |
                         MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID)) != 0U
  ) {
    set_thread_error("mln_vulkan_device_options.fields contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_selection.presentation =
    (options->fields & MLN_VULKAN_DEVICE_OPTION_PRESENTATION) != 0U &&
    options->presentation;
  if ((options->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID) != 0U) {
    const auto status = copy_device_id(
      options->device_uuid,
      "mln_vulkan_device_options.device_uuid must hold 16 bytes",
      out_selection.uuid
    );
    if (status != MLN_STATUS_OK) return status;
  }
  if ((options->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID) != 0U) {
    const auto status = copy_device_id(
      options->device_luid,
      "mln_vulkan_device_options.device_luid must hold 8 bytes",
      out_selection.luid
    );
    if (status != MLN_STATUS_OK) return status;
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto vulkan_device_create(
  const mln_vulkan_device_options* options, mln_vulkan_device* out_device
) -> mln_status {
  auto selection = VulkanDeviceSelection{};
  const auto options_status = vulkan_device_selection(options, selection);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  if (out_device == nullptr || *out_device != MLN_HANDLE_NULL) {
    set_thread_error("out_device must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto context = std::shared_ptr<const mln_vulkan_context_descriptor>{};
  const auto create_status = create_backend_vulkan_device(selection, context);
  if (create_status != MLN_STATUS_OK) {
    return create_status;
  }
  *out_device = vulkan_device_table().insert(
    std::make_shared<VulkanDeviceObject>(VulkanDeviceObject{
      .context = std::move(context), .presentation = selection.presentation
    })
  );
  return MLN_STATUS_OK;
}

auto vulkan_device_get_context(
  mln_vulkan_device device, mln_vulkan_device_context_result* out_result
) -> mln_status {
  const auto object = vulkan_device_table().lease(device);
  if (object == nullptr) {
    return recorded_handle_fault_status();
  }
  if (
    out_result == nullptr ||
    out_result->size < sizeof(mln_vulkan_device_context_result)
  ) {
    set_thread_error("out_result must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_result->context = *object->context;
  return MLN_STATUS_OK;
}

auto vulkan_device_release(mln_vulkan_device device) -> void {
  if (device == MLN_HANDLE_NULL) {
    return;
  }
  // The device itself goes once no session holds it either.
  vulkan_device_table().remove(device);
}

auto window_presentation_device(
  mln_vulkan_device device,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status {
  if (device == MLN_HANDLE_NULL) return MLN_STATUS_OK;
#if defined(MLN_RENDER_BACKEND_VULKAN)
  const auto object = vulkan_device_table().lease(device);
  if (object == nullptr) return recorded_handle_fault_status();
  if (!object->presentation) {
    set_thread_error(
      "the Vulkan device can't present, because it was created without "
      "presentation"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  out_context = object->context;
  return MLN_STATUS_OK;
#else
  static_cast<void>(out_context);
  set_thread_error("a build without Vulkan presents with no Vulkan device");
  return MLN_STATUS_INVALID_ARGUMENT;
#endif
}

}  // namespace mln::core
