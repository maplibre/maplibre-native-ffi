#define MLN_BUILDING_C

#include <cstddef>
#include <cstdint>

#include "c_api/boundary.hpp"
#include "geojson/geojson.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_rendered_feature_query_options_default() noexcept
  -> mln_rendered_feature_query_options {
  return mln_rendered_feature_query_options{
    .size = sizeof(mln_rendered_feature_query_options),
    .fields = 0,
    .layer_ids = nullptr,
    .layer_id_count = 0,
    .filter = nullptr
  };
}

auto mln_source_feature_query_options_default() noexcept
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

auto mln_render_session_resize(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_resize(
      session, width, height, scale_factor
    );
  });
}

auto mln_render_session_render_update(
  mln_render_session session, mln_render_result* out_result,
  bool* out_needs_repaint
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_render_update(
      session, out_result, out_needs_repaint
    );
  });
}

auto mln_render_session_detach(mln_render_session session) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_detach(session);
  });
}

auto mln_render_session_destroy(mln_render_session session) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_destroy(session);
  });
}

auto mln_render_session_reduce_memory_use(mln_render_session session) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_reduce_memory_use(session);
  });
}

auto mln_render_session_clear_data(mln_render_session session) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_clear_data(session);
  });
}

auto mln_render_session_dump_debug_logs(mln_render_session session) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_dump_debug_logs(session);
  });
}

auto mln_render_session_set_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector,
  mln_buffer_view state
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_set_feature_state(
      session, selector, state
    );
  });
}

auto mln_render_session_get_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector,
  mln_buffer* out_state
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_get_feature_state(
      session, selector, out_state
    );
  });
}

auto mln_render_session_remove_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_remove_feature_state(session, selector);
  });
}

auto mln_queried_feature_default() noexcept -> mln_queried_feature {
  return mln_queried_feature{
    .size = sizeof(mln_queried_feature),
    .fields = 0,
    .feature = {},
    .source_id = {},
    .source_layer_id = {},
    .state = {}
  };
}

auto mln_queried_feature_list_count(
  mln_queried_feature_list list, size_t* out_count
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::queried_feature_list_count(list, out_count);
  });
}

auto mln_queried_feature_list_get(
  mln_queried_feature_list list, size_t index, mln_queried_feature* out_feature
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::queried_feature_list_get(list, index, out_feature);
  });
}

auto mln_queried_feature_list_destroy(mln_queried_feature_list list) noexcept
  -> void {
  mln::core::queried_feature_list_destroy(list);
}

auto mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_queried_feature_list* out_result
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_rendered_features(
      session, geometry, options, out_result
    );
  });
}

auto mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  mln_queried_feature_list* out_result
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_source_features(
      session, source_id, options, out_result
    );
  });
}

auto mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_buffer* out_result
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_feature_extensions(
      session, source_id, feature, extension, extension_field, arguments,
      out_result
    );
  });
}
