#include <memory>
#include <string>
#include <utility>

#include <mln/util/logging.hpp>

#include <Foundation/NSArray.hpp>
#include <Foundation/NSSharedPtr.hpp>
#include <Foundation/NSString.hpp>
#include <Metal/MTLDevice.hpp>
#include <TargetConditionals.h>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/graphics_device.hpp"
#include "render/metal/metal_texture_session.hpp"
#include "render/render_session_common.hpp"

namespace mln::core {
namespace {

class MetalGraphicsDevice final : public GraphicsDevice {
 public:
  explicit MetalGraphicsDevice(NS::SharedPtr<MTL::Device> device)
      : device_(std::move(device)) {}

  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    out_backend = create_metal_owned_texture_backend(device_.get(), size, 1);
    return MLN_STATUS_OK;
  }

 private:
  NS::SharedPtr<MTL::Device> device_;
};

auto named_device(uint32_t index, NS::SharedPtr<MTL::Device>& out_device)
  -> mln_status {
#if TARGET_OS_OSX
  const auto devices = NS::TransferPtr(MTL::CopyAllDevices());
  const auto count = devices ? devices->count() : NS::UInteger{0};
  if (index < count) {
    out_device = NS::RetainPtr(devices->object<MTL::Device>(index));
    return MLN_STATUS_OK;
  }
  const auto message = "adapter_index " + std::to_string(index) +
                       " is past the " + std::to_string(count) +
                       " Metal devices";
#else
  // Only macOS lists Metal devices, so the one adapter elsewhere is the system
  // default device.
  if (index == 0) {
    out_device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    return MLN_STATUS_OK;
  }
  const auto message = "adapter_index " + std::to_string(index) +
                       " is past the one Metal device on this platform";
#endif
  set_thread_error(message.c_str());
  return MLN_STATUS_NATIVE_ERROR;
}

}  // namespace

auto create_backend_graphics_device(
  const mln_graphics_device_options& options,
  std::shared_ptr<GraphicsDevice>& out_device
) -> mln_status {
  auto device = NS::SharedPtr<MTL::Device>{};
  if ((options.fields & MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX) != 0U) {
    const auto status = named_device(options.adapter_index, device);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  } else {
    device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
  }
  if (!device) {
    set_thread_error("Metal has no device");
    return MLN_STATUS_NATIVE_ERROR;
  }
  mln::Log::Info(
    mln::Event::Render, std::string{"graphics device uses Metal device "} +
                          device->name()->utf8String()
  );
  out_device = std::make_shared<MetalGraphicsDevice>(std::move(device));
  return MLN_STATUS_OK;
}

}  // namespace mln::core
