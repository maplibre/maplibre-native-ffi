#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::render_session_query_rendered_features_start(
      session, geometry, options, completion
    );
  });
}

auto mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::render_session_query_source_features_start(
      session, source_id, options, completion
    );
  });
}

auto mln_render_session_get_cluster_children(
  mln_render_session session, mln_buffer_view source_id, uint32_t cluster_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::render_session_get_cluster_children_start(
      session, source_id, cluster_id, completion
    );
  });
}

auto mln_render_session_get_cluster_leaves(
  mln_render_session session, mln_buffer_view source_id, uint32_t cluster_id,
  uint32_t limit, uint32_t offset, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::render_session_get_cluster_leaves_start(
      session, source_id, cluster_id, limit, offset, completion
    );
  });
}

auto mln_render_session_get_cluster_expansion_zoom(
  mln_render_session session, mln_buffer_view source_id, uint32_t cluster_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::render_session_get_cluster_expansion_zoom_start(
      session, source_id, cluster_id, completion
    );
  });
}
