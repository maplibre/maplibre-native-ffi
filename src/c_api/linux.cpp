#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/texture_session.hpp"

// Linux and FreeBSD sessions can render into dmabufs that they allocate.
// Android and OpenHarmony define __linux__ too.
#if (defined(__linux__) || defined(__FreeBSD__)) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
#define MLN_FFI_DMA_BUFS 1
#include "render/dma_buf_session.hpp"
#endif

auto mln_map_attach_dma_buf_ring(
  mln_map map, const mln_dma_buf_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef MLN_FFI_DMA_BUFS
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status = mln::core::validate_dma_buf_ring_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("dmabufs are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::dma_buf_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_acquired_frame_get_dma_buf(
  mln_acquired_frame frame, mln_dma_buf_frame* out_frame,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::acquired_frame_get_dma_buf(frame, out_frame);
  });
}
