#include <memory>
#include <optional>
#include <utility>

#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"
#include "render/render_session_common.hpp"
#include "render/vulkan/vulkan_device.hpp"
#include "render/vulkan/vulkan_texture_session.hpp"
#include "render/vulkan_device.hpp"

namespace mln::core {
namespace {

class VulkanOffscreenDevice final : public OffscreenDevice {
 public:
  explicit VulkanOffscreenDevice(
    std::shared_ptr<const mln_vulkan_context_descriptor> context
  )
      : context_(std::move(context)) {}

  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    out_backend = create_vulkan_offscreen_texture_backend(*context_, size);
    return MLN_STATUS_OK;
  }

 private:
  std::shared_ptr<const mln_vulkan_context_descriptor> context_;
};

}  // namespace

auto create_backend_offscreen_device(
  const mln_offscreen_device_options& options,
  std::shared_ptr<OffscreenDevice>& out_device
) -> mln_status {
  const auto adapter_index =
    (options.fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0U
      ? std::optional{options.adapter_index}
      : std::nullopt;
  auto context = std::shared_ptr<const mln_vulkan_context_descriptor>{};
  const auto status =
    create_vulkan_device(VulkanDeviceSelection{}, adapter_index, context);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  out_device = std::make_shared<VulkanOffscreenDevice>(std::move(context));
  return MLN_STATUS_OK;
}

}  // namespace mln::core
