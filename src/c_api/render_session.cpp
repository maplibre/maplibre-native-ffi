#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_frame_demand_default() noexcept -> mln_frame_demand {
  return mln::c_api::sized<mln_frame_demand>();
}
auto mln_render_session_attach_options_default() noexcept
  -> mln_render_session_attach_options {
  return mln::c_api::sized<mln_render_session_attach_options>();
}

auto mln_gpu_sync_default() noexcept -> mln_gpu_sync {
  return mln::c_api::sized<mln_gpu_sync>();
}

auto mln_render_session_get_capabilities(
  mln_render_session session, mln_render_session_capabilities* out_capabilities,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_get_capabilities(
      session, out_capabilities
    );
  });
}

auto mln_render_session_get_snapshot(
  mln_render_session session, mln_render_session_snapshot* out_snapshot,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_get_snapshot(session, out_snapshot);
  });
}

auto mln_render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_request_frame(session, demand);
  });
}

auto mln_render_session_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch,
  mln_render_frame_batch_view* out_view, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_drain_frame_results(
      session, out_batch, out_view
    );
  });
}

void mln_render_frame_batch_release(mln_render_frame_batch batch) noexcept {
  mln::core::render_frame_batch_release(batch);
}

auto mln_render_session_acquire_frame(
  mln_render_session session, mln_acquired_frame* out_frame,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_acquire_frame(session, out_frame);
  });
}

auto mln_acquired_frame_get_result(
  mln_acquired_frame frame, mln_render_frame_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_get_result(frame, out_result);
  });
}

auto mln_acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_get_producer_sync(frame, out_sync);
  });
}

auto mln_acquired_frame_release(
  mln_acquired_frame frame, const mln_gpu_sync* consumer_completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_release(frame, consumer_completion);
  });
}

auto mln_render_session_resize(
  mln_render_session session, mln_logical_extent extent,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_resize_start(session, extent, completion);
  });
}

auto mln_render_session_barrier(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_barrier_start(session, completion);
  });
}

auto mln_render_session_reduce_memory_use(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_maintenance_start(
      session, mln::core::RenderSessionMaintenance::ReduceMemoryUse, completion
    );
  });
}

auto mln_render_session_clear_data(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_maintenance_start(
      session, mln::core::RenderSessionMaintenance::ClearData, completion
    );
  });
}

auto mln_render_session_dump_debug_logs(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_maintenance_start(
      session, mln::core::RenderSessionMaintenance::DumpDebugLogs, completion
    );
  });
}

auto mln_render_session_service_driver_work(
  mln_render_session session, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_service_driver_work(session);
  });
}

auto mln_render_session_detach(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_detach_start(session, completion);
  });
}

auto mln_render_session_abandon(
  mln_render_session session, mln_render_abandon_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_abandon(session, out_result);
  });
}

auto mln_render_session_dispose(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_dispose(session, completion);
  });
}

auto mln_acquired_frame_discard(
  mln_acquired_frame frame, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_discard(frame);
  });
}

auto mln_acquired_frame_view_begin(
  mln_acquired_frame frame, void** out_scope, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_view_begin(frame, out_scope);
  });
}
void mln_acquired_frame_view_end(void* scope) noexcept {
  mln::core::acquired_frame_view_end(scope);
}
