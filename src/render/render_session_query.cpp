#include <any>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <mln/style/source_impl.hpp>
#include <mln/style/sources/geojson_source_impl.hpp>
#include <mln/util/geojson.hpp>

#include "completion/completion_result.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "render/render_session_common.hpp"
#include "style/style_value.hpp"

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

// Keep the copied query results alive through their completion callbacks.
using OwnedRenderedFeatureList = std::shared_ptr<const RenderedFeatureList>;
using OwnedSourceFeatureList = std::shared_ptr<const SourceFeatureList>;

auto string_view_of(const std::string& string) -> mln_buffer_view {
  return {.data = string.data(), .size = string.size()};
}

auto rendered_feature_view(const RenderedFeatureRecord& record)
  -> mln_rendered_feature {
  return mln_rendered_feature{
    .feature = string_view_of(record.feature),
    .layer_id = string_view_of(record.layer_id),
    .source_id = string_view_of(record.source_id),
    .source_layer_id = string_view_of(record.source_layer_id),
    .state = string_view_of(record.state)
  };
}

auto source_feature_view(const SourceFeatureRecord& record)
  -> mln_source_feature {
  return mln_source_feature{
    .feature = string_view_of(record.feature),
    .source_layer_id = string_view_of(record.source_layer_id)
  };
}

// Completes a query whose driver task failed, or succeeded without a result.
auto complete_query_failure(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic
) -> void {
  complete_failure(
    completion, status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
    status == MLN_STATUS_OK ? "render query produced an invalid result"
                            : std::move(diagnostic)
  );
}

template <auto Function, class List, auto View>
auto complete_feature_list(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic, std::any result
) -> void {
  auto* list = std::any_cast<std::shared_ptr<const List>>(&result);
  if (status != MLN_STATUS_OK || list == nullptr || *list == nullptr) {
    complete_query_failure(completion, status, std::move(diagnostic));
    return;
  }
  auto features = std::vector<decltype(View((*list)->features.front()))>{};
  features.reserve((*list)->features.size());
  for (const auto& record : (*list)->features) {
    features.push_back(View(record));
  }
  completion->resolve([list = std::move(*list), features = std::move(features)](
                        const CompletionSink& descriptor
                      ) {
    static_cast<void>(list);
    CompletionValue<Function>::deliver(descriptor, features);
  });
}

template <auto Function>
auto complete_query_buffer(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic, std::any result
) -> void {
  auto* bytes = std::any_cast<std::string>(&result);
  if (status != MLN_STATUS_OK || bytes == nullptr) {
    complete_query_failure(completion, status, std::move(diagnostic));
    return;
  }
  completion->resolve([bytes =
                         std::move(*bytes)](const CompletionSink& descriptor) {
    CompletionValue<Function>::deliver(
      descriptor, {.data = bytes.data(), .size = bytes.size()}
    );
  });
}

template <auto Function>
auto complete_query_value(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic, std::any result
) -> void {
  auto* value =
    std::any_cast<typename CompletionValue<Function>::Type>(&result);
  if (status != MLN_STATUS_OK || value == nullptr) {
    complete_query_failure(completion, status, std::move(diagnostic));
    return;
  }
  CompletionValue<Function>::complete(completion, *value);
}

// Copies a source ID for a driver task.
auto copy_source_id(mln_buffer_view source_id, std::string& out) -> mln_status {
  const auto status = copy_view(source_id, out);
  if (status != MLN_STATUS_OK) return status;
  if (out.empty()) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

using ClusterIndex = std::shared_ptr<mln::style::GeoJSONData>;

// Finds the index of the clustered GeoJSON source named `source_id` among the
// sources the renderer holds. A source whose data hasn't
// loaded has no index, so it knows no cluster.
auto find_cluster_index(
  mln_render_session_object& target, const std::string& source_id,
  ClusterIndex& out_index
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(target.self, live);
  if (status != MLN_STATUS_OK) return status;
  const mln::style::Source::Impl* source = nullptr;
  if (live->renderer_sources) {
    for (const auto& impl : **live->renderer_sources) {
      if (impl->id == source_id) {
        source = &*impl;
        break;
      }
    }
  }
  if (source == nullptr) {
    const auto message =
      "the session's last render has no source named " + source_id;
    set_thread_error(message.c_str());
    return MLN_STATUS_NOT_FOUND;
  }
  if (source->type != mln::style::SourceType::GeoJSON) {
    const auto message = "source " + source_id + " is not a GeoJSON source";
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto& geojson =
    static_cast<const mln::style::GeoJSONSource::Impl&>(*source);
  if (!geojson.getOptions()->cluster) {
    const auto message = "GeoJSON source " + source_id + " does not cluster";
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_index = geojson.getData().lock();
  return MLN_STATUS_OK;
}

// Reads a cluster's children, which also checks that the index knows the
// cluster. Supercluster throws for most IDs it doesn't know. It stores a
// cluster's zoom plus one in the low five bits of the ID, so no cluster ID has
// zeros there, yet an ID of 0 would find zoom 0's points, whose parent ID
// defaults to 0, as children. An index of no points answers every ID with no
// children.
auto read_cluster_children(
  const ClusterIndex& index, std::uint32_t cluster_id,
  mln::FeatureCollection& out_children
) -> mln_status {
  constexpr auto zoom_bits = std::uint32_t{0x1F};
  if (index != nullptr && (cluster_id & zoom_bits) != 0) {
    try {
      out_children = index->getChildren(cluster_id);
    } catch (const std::runtime_error&) {
      out_children.clear();
    }
    if (!out_children.empty()) return MLN_STATUS_OK;
  }
  const auto message =
    "the source has no cluster with ID " + std::to_string(cluster_id);
  set_thread_error(message.c_str());
  return MLN_STATUS_NOT_FOUND;
}

// Submits a driver task that finds the cluster `cluster_id` in the source
// named `source_id`, then passes the index and the cluster's children to
// `read`, which stores the query's result.
template <auto Complete, class Read>
auto enqueue_cluster_query(
  mln_render_session session, mln_buffer_view source_id,
  std::uint32_t cluster_id, const mln_completion* completion, Read read
) -> mln_status {
  auto source = std::string{};
  const auto source_status = copy_source_id(source_id, source);
  if (source_status != MLN_STATUS_OK) return source_status;
  return enqueue_driver_result_operation(
    session,
    [source = std::move(source), cluster_id,
     read](mln_render_session_object& target, std::any& result) {
      auto index = ClusterIndex{};
      auto status = find_cluster_index(target, source, index);
      auto children = mln::FeatureCollection{};
      if (status == MLN_STATUS_OK) {
        status = read_cluster_children(index, cluster_id, children);
      }
      if (status == MLN_STATUS_OK) {
        result = read(*index, cluster_id, children);
      }
      return status;
    },
    completion, Complete
  );
}

// Copies a query filter once it parses, so the submission rejects a filter
// that the driver could not apply. An empty filter is no filter.
auto copy_filter(const mln_buffer_view& filter, std::string& out)
  -> mln_status {
  const auto status = copy_view(filter, out);
  if (status != MLN_STATUS_OK) return status;
  if (filter.size == 0) return MLN_STATUS_OK;
  if (!to_native_style_filter(&filter)) return MLN_STATUS_INVALID_ARGUMENT;
  return MLN_STATUS_OK;
}

struct CopiedRenderedOptions {
  mln_rendered_feature_query_options value{};
  std::vector<std::string> layer_ids;
  std::string filter;
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
  return copy_filter(input->filter, out->filter);
}

struct CopiedSourceOptions {
  mln_source_feature_query_options value{};
  std::vector<std::string> layer_ids;
  std::string filter;
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
  return copy_filter(input->filter, out->filter);
}

auto make_views(const std::vector<std::string>& strings)
  -> std::vector<mln_buffer_view> {
  auto views = std::vector<mln_buffer_view>{};
  views.reserve(strings.size());
  for (const auto& string : strings) {
    views.push_back(mln_buffer_view{string.data(), string.size()});
  }
  return views;
}

}  // namespace

auto render_session_query_rendered_features_start(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  const mln_completion* completion
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
    session,
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
      if (copied_options) {
        views = make_views(copied_options->layer_ids);
        copied_options->value.layer_ids = views.data();
        copied_options->value.filter = mln_buffer_view{
          copied_options->filter.data(), copied_options->filter.size()
        };
        option_pointer = &copied_options->value;
      }
      auto list = OwnedRenderedFeatureList{};
      const auto status = render_session_query_rendered_features(
        target.self, &copied_geometry, option_pointer, list
      );
      if (status == MLN_STATUS_OK) result = std::move(list);
      return status;
    },
    completion,
    complete_feature_list<
      &mln_render_session_query_rendered_features, RenderedFeatureList,
      rendered_feature_view>
  );
}

auto render_session_query_source_features_start(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  const mln_completion* completion
) -> mln_status {
  auto source = std::string{};
  const auto source_status = copy_source_id(source_id, source);
  if (source_status != MLN_STATUS_OK) return source_status;
  auto copied_options = std::optional<CopiedSourceOptions>{};
  const auto options_status = copy_source_options(options, copied_options);
  if (options_status != MLN_STATUS_OK) return options_status;
  return enqueue_driver_result_operation(
    session,
    [source = std::move(source), copied_options = std::move(copied_options)](
      mln_render_session_object& target, std::any& result
    ) mutable {
      const mln_source_feature_query_options* option_pointer = nullptr;
      auto views = std::vector<mln_buffer_view>{};
      if (copied_options) {
        views = make_views(copied_options->layer_ids);
        copied_options->value.source_layer_ids = views.data();
        copied_options->value.filter = mln_buffer_view{
          copied_options->filter.data(), copied_options->filter.size()
        };
        option_pointer = &copied_options->value;
      }
      auto list = OwnedSourceFeatureList{};
      const auto status = render_session_query_source_features(
        target.self, mln_buffer_view{source.data(), source.size()},
        option_pointer, list
      );
      if (status == MLN_STATUS_OK) result = std::move(list);
      return status;
    },
    completion,
    complete_feature_list<
      &mln_render_session_query_source_features, SourceFeatureList,
      source_feature_view>
  );
}

auto render_session_get_cluster_children_start(
  mln_render_session session, mln_buffer_view source_id,
  std::uint32_t cluster_id, const mln_completion* completion
) -> mln_status {
  return enqueue_cluster_query<
    complete_query_buffer<&mln_render_session_get_cluster_children>>(
    session, source_id, cluster_id, completion,
    [](
      mln::style::GeoJSONData&, std::uint32_t,
      const mln::FeatureCollection& children
    ) { return serialize_feature_collection(children); }
  );
}

auto render_session_get_cluster_leaves_start(
  mln_render_session session, mln_buffer_view source_id,
  std::uint32_t cluster_id, std::uint32_t limit, std::uint32_t offset,
  const mln_completion* completion
) -> mln_status {
  return enqueue_cluster_query<
    complete_query_buffer<&mln_render_session_get_cluster_leaves>>(
    session, source_id, cluster_id, completion,
    [limit, offset](
      mln::style::GeoJSONData& index, std::uint32_t id,
      const mln::FeatureCollection&
    ) {
      return serialize_feature_collection(index.getLeaves(id, limit, offset));
    }
  );
}

auto render_session_get_cluster_expansion_zoom_start(
  mln_render_session session, mln_buffer_view source_id,
  std::uint32_t cluster_id, const mln_completion* completion
) -> mln_status {
  return enqueue_cluster_query<
    complete_query_value<&mln_render_session_get_cluster_expansion_zoom>>(
    session, source_id, cluster_id, completion,
    [](
      mln::style::GeoJSONData& index, std::uint32_t id,
      const mln::FeatureCollection&
    ) { return std::uint32_t{index.getClusterExpansionZoom(id)}; }
  );
}

}  // namespace mln::core
