#pragma once

#include <memory>

#include <mln/util/size.hpp>

#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

class TextureSessionBackend;

// The graphics objects of one device, made by this build's backend. They are
// immutable after creation, so any thread may read them. The last owner
// destroys them: the handle table, or the backend of the last texture session
// that renders on them.
class OffscreenDevice {
 public:
  OffscreenDevice() = default;
  OffscreenDevice(const OffscreenDevice&) = delete;
  auto operator=(const OffscreenDevice&) -> OffscreenDevice& = delete;
  OffscreenDevice(OffscreenDevice&&) = delete;
  auto operator=(OffscreenDevice&&) -> OffscreenDevice& = delete;
  virtual ~OffscreenDevice() = default;

  // Makes the backend of a readback-only session with a one-slot ring on
  // this device. Runs on the session's core worker.
  virtual auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status = 0;
};

struct OffscreenDeviceObject {
  std::shared_ptr<OffscreenDevice> device;
};

template <>
struct HandleTraits<OffscreenDeviceObject> {
  static constexpr auto kind = HandleKind::OffscreenDevice;
  static constexpr auto leasable = true;
};

// Each backend defines this, and gets validated options. It runs on any
// thread, except in a browser WebGPU build: there only a session's core worker
// calls it, and the device it makes serves that worker alone.
auto create_backend_offscreen_device(
  const mln_offscreen_device_options& options,
  std::shared_ptr<OffscreenDevice>& out_device
) -> mln_status;

auto offscreen_device_create(
  const mln_offscreen_device_options* options, mln_offscreen_device* out_device
) -> mln_status;
auto offscreen_device_release(mln_offscreen_device device) -> void;
auto offscreen_attach_start(
  mln_map map, const mln_offscreen_descriptor* descriptor,
  const mln_render_session_attach_options* options, mln_offscreen_device device,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
