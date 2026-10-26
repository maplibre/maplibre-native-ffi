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
class GraphicsDevice {
 public:
  GraphicsDevice() = default;
  GraphicsDevice(const GraphicsDevice&) = delete;
  auto operator=(const GraphicsDevice&) -> GraphicsDevice& = delete;
  GraphicsDevice(GraphicsDevice&&) = delete;
  auto operator=(GraphicsDevice&&) -> GraphicsDevice& = delete;
  virtual ~GraphicsDevice() = default;

  // Makes the backend of a readback-only session with a one-slot ring on
  // this device. Runs on the session's core worker.
  virtual auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status = 0;
};

struct GraphicsDeviceObject {
  std::shared_ptr<GraphicsDevice> device;
};

template <>
struct HandleTraits<GraphicsDeviceObject> {
  static constexpr auto kind = HandleKind::GraphicsDevice;
  static constexpr auto leasable = true;
};

// Creates the device of this build's backend from validated options. Each
// backend defines it; a build whose backend creates no device reports
// MLN_STATUS_UNSUPPORTED.
auto create_backend_graphics_device(
  const mln_graphics_device_options& options,
  std::shared_ptr<GraphicsDevice>& out_device
) -> mln_status;

auto graphics_device_create(
  const mln_graphics_device_options* options, mln_graphics_device* out_device
) -> mln_status;
auto graphics_device_release(mln_graphics_device device) -> void;
auto graphics_device_texture_attach_start(
  mln_map map, mln_graphics_device device,
  const mln_graphics_device_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
