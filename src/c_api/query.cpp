#define MLN_BUILDING_C

#include <any>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "bytes/buffer.hpp"
#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
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

auto mln_render_session_query_rendered_features_start(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_rendered_features_start(
      session, geometry, options, out_operation
    );
  });
}

auto mln_render_session_query_source_features_start(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_source_features_start(
      session, source_id, options, out_operation
    );
  });
}

auto mln_render_session_query_feature_extensions_start(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_session_query_feature_extensions_start(
      session, source_id, feature, extension, extension_field, arguments,
      out_operation
    );
  });
}

auto mln_render_query_features_take_result(
  mln_operation operation, mln_queried_feature_list* out_result
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_query_features_take_result(operation, out_result);
  });
}

auto mln_render_query_take_result(
  mln_operation operation, mln_buffer* out_result
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::render_query_take_result(operation, out_result);
  });
}

auto mln_queried_feature_default(void) noexcept -> mln_queried_feature {
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

namespace mln::core {
namespace {
auto validate_screen_point(mln_screen_point point) -> mln_status {
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
    set_thread_error("screen point coordinates must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto copy_view(mln_buffer_view view, std::string& out) -> mln_status {
  if (view.size != 0 && view.data == nullptr) {
    set_thread_error("buffer view data must not be null when size is nonzero");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto* bytes = static_cast<const char*>(view.data);
  out.assign(bytes == nullptr ? "" : bytes, view.size);
  return MLN_STATUS_OK;
}

auto take_buffer_bytes(mln_buffer buffer, std::any& out) -> mln_status {
  mln_buffer_view view{};
  const auto status = buffer_get(buffer, &view);
  if (status != MLN_STATUS_OK) {
    buffer_destroy(buffer);
    return status;
  }
  auto bytes = std::string{};
  const auto copy_status = copy_view(view, bytes);
  buffer_destroy(buffer);
  if (copy_status != MLN_STATUS_OK) {
    return copy_status;
  }
  out = std::move(bytes);
  return MLN_STATUS_OK;
}

// Owns a queried-feature list handle inside an operation result, so a result
// that is discarded or abandoned instead of taken still destroys the list.
using OwnedQueriedFeatureList = std::shared_ptr<mln_queried_feature_list>;

auto store_feature_list(mln_queried_feature_list list, std::any& out)
  -> mln_status {
  try {
    out = OwnedQueriedFeatureList{
      new mln_queried_feature_list{list}, [](mln_queried_feature_list* owned) {
        queried_feature_list_destroy(*owned);
        delete owned;
      }
    };
  } catch (...) {
    queried_feature_list_destroy(list);
    throw;
  }
  return MLN_STATUS_OK;
}

struct CopiedRenderedOptions {
  mln_rendered_feature_query_options value{};
  std::vector<std::string> layer_ids;
  std::optional<std::string> filter;
};

auto copy_rendered_options(
  const mln_rendered_feature_query_options* input,
  std::optional<CopiedRenderedOptions>& out
) -> mln_status {
  if (input == nullptr) {
    return MLN_STATUS_OK;
  }
  if (input->size < sizeof(*input)) {
    set_thread_error("rendered feature query options size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  if ((input->fields & ~known_fields) != 0) {
    set_thread_error("rendered feature query options have unknown fields");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out.emplace();
  out->value = *input;
  if (
    (input->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0 &&
    input->layer_id_count != 0 && input->layer_ids == nullptr
  ) {
    set_thread_error(
      "layer_ids must not be null when layer_id_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((input->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0) {
    out->layer_ids.resize(input->layer_id_count);
    for (std::size_t i = 0; i < input->layer_id_count; ++i) {
      const auto status = copy_view(input->layer_ids[i], out->layer_ids[i]);
      if (status != MLN_STATUS_OK) return status;
    }
  }
  if (input->filter != nullptr) {
    out->filter.emplace();
    return copy_view(*input->filter, *out->filter);
  }
  return MLN_STATUS_OK;
}

struct CopiedSourceOptions {
  mln_source_feature_query_options value{};
  std::vector<std::string> layer_ids;
  std::optional<std::string> filter;
};

auto copy_source_options(
  const mln_source_feature_query_options* input,
  std::optional<CopiedSourceOptions>& out
) -> mln_status {
  if (input == nullptr) return MLN_STATUS_OK;
  if (input->size < sizeof(*input)) {
    set_thread_error("source feature query options size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  if ((input->fields & ~known_fields) != 0) {
    set_thread_error("source feature query options have unknown fields");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out.emplace();
  out->value = *input;
  if (
    (input->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0 &&
    input->source_layer_id_count != 0 && input->source_layer_ids == nullptr
  ) {
    set_thread_error(
      "source_layer_ids must not be null when source_layer_id_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((input->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0) {
    out->layer_ids.resize(input->source_layer_id_count);
    for (std::size_t i = 0; i < input->source_layer_id_count; ++i) {
      const auto status =
        copy_view(input->source_layer_ids[i], out->layer_ids[i]);
      if (status != MLN_STATUS_OK) return status;
    }
  }
  if (input->filter != nullptr) {
    out->filter.emplace();
    return copy_view(*input->filter, *out->filter);
  }
  return MLN_STATUS_OK;
}

template <typename Options>
auto make_views(const std::vector<std::string>& strings, Options& options)
  -> std::vector<mln_buffer_view> {
  auto views = std::vector<mln_buffer_view>{};
  views.reserve(strings.size());
  for (const auto& string : strings) {
    views.push_back(mln_buffer_view{string.data(), string.size()});
  }
  (void)options;
  return views;
}

}  // namespace

auto render_session_query_rendered_features_start(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_operation* out_operation
) -> mln_status {
  if (geometry == nullptr || geometry->size < sizeof(*geometry)) {
    set_thread_error("rendered query geometry is null or too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto copied_geometry = *geometry;
  auto points = std::vector<mln_screen_point>{};
  switch (geometry->type) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      if (validate_screen_point(geometry->data.point) != MLN_STATUS_OK) {
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX:
      if (
        validate_screen_point(geometry->data.box.min) != MLN_STATUS_OK ||
        validate_screen_point(geometry->data.box.max) != MLN_STATUS_OK
      ) {
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING: {
      const auto& line = geometry->data.line_string;
      if (line.point_count == 0) {
        set_thread_error("query line string must contain points");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (line.points == nullptr) {
        set_thread_error("line string points must not be null");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      points.reserve(line.point_count);
      for (std::size_t i = 0; i < line.point_count; ++i) {
        if (validate_screen_point(line.points[i]) != MLN_STATUS_OK) {
          return MLN_STATUS_INVALID_ARGUMENT;
        }
        points.push_back(line.points[i]);
      }
      break;
    }
    default:
      set_thread_error("rendered query geometry type is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto copied_options = std::optional<CopiedRenderedOptions>{};
  const auto options_status = copy_rendered_options(options, copied_options);
  if (options_status != MLN_STATUS_OK) return options_status;

  return enqueue_driver_result_operation(
    session, RENDER_OPERATION_QUERY_FEATURES,
    [copied_geometry, points = std::move(points),
     copied_options = std::move(copied_options)](
      mln_render_session_object& target, std::any& result
    ) mutable {
      if (
        copied_geometry.type == MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING
      ) {
        copied_geometry.data.line_string =
          mln_screen_line_string{points.data(), points.size()};
      }
      const mln_rendered_feature_query_options* option_pointer = nullptr;
      auto views = std::vector<mln_buffer_view>{};
      auto filter_view = mln_buffer_view{};
      if (copied_options) {
        views = make_views(copied_options->layer_ids, copied_options->value);
        copied_options->value.layer_ids = views.data();
        if (copied_options->filter) {
          filter_view = mln_buffer_view{
            copied_options->filter->data(), copied_options->filter->size()
          };
          copied_options->value.filter = &filter_view;
        }
        option_pointer = &copied_options->value;
      }
      auto list = mln_queried_feature_list{MLN_HANDLE_NULL};
      const auto status = render_session_query_rendered_features(
        target.self, &copied_geometry, option_pointer, &list
      );
      return status == MLN_STATUS_OK ? store_feature_list(list, result)
                                     : status;
    },
    out_operation
  );
}

auto render_session_query_source_features_start(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options, mln_operation* out_operation
) -> mln_status {
  auto source = std::string{};
  const auto source_status = copy_view(source_id, source);
  if (source_status != MLN_STATUS_OK) return source_status;
  auto copied_options = std::optional<CopiedSourceOptions>{};
  const auto options_status = copy_source_options(options, copied_options);
  if (options_status != MLN_STATUS_OK) return options_status;
  return enqueue_driver_result_operation(
    session, RENDER_OPERATION_QUERY_FEATURES,
    [source = std::move(source), copied_options = std::move(copied_options)](
      mln_render_session_object& target, std::any& result
    ) mutable {
      const mln_source_feature_query_options* option_pointer = nullptr;
      auto views = std::vector<mln_buffer_view>{};
      auto filter_view = mln_buffer_view{};
      if (copied_options) {
        views = make_views(copied_options->layer_ids, copied_options->value);
        copied_options->value.source_layer_ids = views.data();
        if (copied_options->filter) {
          filter_view = mln_buffer_view{
            copied_options->filter->data(), copied_options->filter->size()
          };
          copied_options->value.filter = &filter_view;
        }
        option_pointer = &copied_options->value;
      }
      auto list = mln_queried_feature_list{MLN_HANDLE_NULL};
      const auto status = render_session_query_source_features(
        target.self, mln_buffer_view{source.data(), source.size()},
        option_pointer, &list
      );
      return status == MLN_STATUS_OK ? store_feature_list(list, result)
                                     : status;
    },
    out_operation
  );
}

auto render_session_query_feature_extensions_start(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_operation* out_operation
) -> mln_status {
  auto copied = std::vector<std::string>(5);
  const mln_buffer_view inputs[] = {
    source_id, feature, extension, extension_field,
    arguments == nullptr ? mln_buffer_view{} : *arguments
  };
  for (std::size_t i = 0; i < copied.size(); ++i) {
    const auto status = copy_view(inputs[i], copied[i]);
    if (status != MLN_STATUS_OK) return status;
  }
  const bool has_arguments = arguments != nullptr;
  return enqueue_driver_result_operation(
    session, RENDER_OPERATION_QUERY,
    [copied = std::move(copied),
     has_arguments](mln_render_session_object& target, std::any& result) {
      const auto view = [&copied](std::size_t i) {
        return mln_buffer_view{copied[i].data(), copied[i].size()};
      };
      const auto argument_view = view(4);
      auto buffer = mln_buffer{MLN_HANDLE_NULL};
      const auto status = render_session_query_feature_extensions(
        target.self, view(0), view(1), view(2), view(3),
        has_arguments ? &argument_view : nullptr, &buffer
      );
      return status == MLN_STATUS_OK ? take_buffer_bytes(buffer, result)
                                     : status;
    },
    out_operation
  );
}

auto render_query_features_take_result(
  mln_operation operation, mln_queried_feature_list* out_result
) -> mln_status {
  if (out_result == nullptr || *out_result != MLN_HANDLE_NULL) {
    set_thread_error(
      "out_result must point to a null queried-feature list handle"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<OwnedQueriedFeatureList>(
    operation, RENDER_OPERATION_QUERY_FEATURES,
    [out_result](OwnedQueriedFeatureList& result) {
      *out_result =
        std::exchange(*result, mln_queried_feature_list{MLN_HANDLE_NULL});
      return MLN_STATUS_OK;
    }
  );
}

auto render_query_take_result(mln_operation operation, mln_buffer* out_result)
  -> mln_status {
  if (out_result == nullptr || *out_result != MLN_HANDLE_NULL) {
    set_thread_error("out_result must point to a null buffer handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<std::string>(
    operation, RENDER_OPERATION_QUERY, [out_result](std::string& result) {
      return create_buffer(std::move(result), out_result);
    }
  );
}

}  // namespace mln::core
