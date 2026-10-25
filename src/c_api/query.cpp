#define MLN_BUILDING_C

#include <cstddef>

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_rendered_feature_query_options_default(void) noexcept
  -> mln_rendered_feature_query_options {
  return mln::c_api::sized<mln_rendered_feature_query_options>();
}

auto mln_source_feature_query_options_default(void) noexcept
  -> mln_source_feature_query_options {
  return mln::c_api::sized<mln_source_feature_query_options>();
}

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

auto mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, mln_buffer_view arguments,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::render_session_query_feature_extensions_start(
      session, source_id, feature, extension, extension_field, arguments,
      completion
    );
  });
}
