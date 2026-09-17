#define MLN_BUILDING_C

#include <cstddef>

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_rendered_feature_query_options_default(void) noexcept
  -> mln_rendered_feature_query_options {
  return mln_rendered_feature_query_options{
    .size = sizeof(mln_rendered_feature_query_options),
    .fields = 0,
    .layer_ids = nullptr,
    .layer_id_count = 0,
    .filter = nullptr
  };
}

auto mln_source_feature_query_options_default(void) noexcept
  -> mln_source_feature_query_options {
  return mln_source_feature_query_options{
    .size = sizeof(mln_source_feature_query_options),
    .fields = 0,
    .source_layer_ids = nullptr,
    .source_layer_id_count = 0,
    .filter = nullptr
  };
}

auto mln_rendered_query_geometry_point(mln_screen_point point) noexcept
  -> mln_rendered_query_geometry {
  return mln_rendered_query_geometry{
    .size = sizeof(mln_rendered_query_geometry),
    .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
    .data = {.point = point}
  };
}

auto mln_rendered_query_geometry_box(mln_screen_box box) noexcept
  -> mln_rendered_query_geometry {
  return mln_rendered_query_geometry{
    .size = sizeof(mln_rendered_query_geometry),
    .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
    .data = {.box = box}
  };
}

auto mln_rendered_query_geometry_line_string(
  const mln_screen_point* points, size_t point_count
) noexcept -> mln_rendered_query_geometry {
  return mln_rendered_query_geometry{
    .size = sizeof(mln_rendered_query_geometry),
    .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
    .data = {.line_string = {.points = points, .point_count = point_count}}
  };
}

auto mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_rendered_features_start(
      session, geometry, options, completion
    );
  });
}

auto mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_source_features_start(
      session, source_id, options, completion
    );
  });
}

auto mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_feature_extensions_start(
      session, source_id, feature, extension, extension_field, arguments,
      completion
    );
  });
}
