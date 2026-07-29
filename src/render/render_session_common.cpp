#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/renderer/query.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/update_parameters.hpp>
#include <mbgl/style/filter.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/geojson.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/util/string.hpp>

#include "render/render_session_common.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "style/style_value.hpp"

namespace mln::core {

struct OwnedQueriedFeatureDescriptor {
  mln_queried_feature queried{};
  mln_feature feature{};
  std::unique_ptr<OwnedGeometryDescriptor> geometry;
  std::vector<std::string> property_keys;
  std::vector<std::unique_ptr<OwnedJsonDescriptor>> property_values;
  std::vector<mln_json_value> property_value_roots;
  std::vector<mln_json_member> properties;
  std::string identifier_string;
  std::string source_id;
  std::string source_layer_id;
  std::unique_ptr<OwnedJsonDescriptor> state;
};

auto render_target_extent_physical_size(
  const mln_render_target_extent* extent, uint32_t* out_width,
  uint32_t* out_height
) -> mln_status {
  if (extent == nullptr) {
    set_thread_error("extent must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_width == nullptr || out_height == nullptr) {
    set_thread_error("out_width and out_height must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    *extent, "extent dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto physical_status = validate_physical_size(
    extent->width, extent->height, extent->scale_factor,
    "scaled extent dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  *out_width = physical_dimension(extent->width, extent->scale_factor);
  *out_height = physical_dimension(extent->height, extent->scale_factor);
  return MLN_STATUS_OK;
}

auto opengl_supported_context_provider_mask() noexcept -> uint32_t {
#if defined(MLN_RENDER_BACKEND_OPENGL) && defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  return MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL;
#elif defined(MLN_RENDER_BACKEND_OPENGL) && defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  return MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL;
#else
  return 0;
#endif
}

auto opengl_context_descriptor_default() noexcept
  -> mln_opengl_context_descriptor {
  auto result = mln_opengl_context_descriptor{
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED,
    .data = {},
  };
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  result.platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL;
  result.data.wgl = mln_wgl_context_descriptor{
    .size = sizeof(mln_wgl_context_descriptor),
    .device_context = nullptr,
    .share_context = nullptr,
    .get_proc_address = nullptr,
  };
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  result.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  result.data.egl = mln_egl_context_descriptor{
    .size = sizeof(mln_egl_context_descriptor),
    .display = nullptr,
    .config = nullptr,
    .share_context = nullptr,
    .get_proc_address = nullptr,
  };
#endif
  return result;
}

auto opengl_owned_texture_descriptor_default() noexcept
  -> mln_opengl_owned_texture_descriptor {
  return mln_opengl_owned_texture_descriptor{
    .size = sizeof(mln_opengl_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = opengl_context_descriptor_default(),
  };
}

auto opengl_borrowed_texture_descriptor_default() noexcept
  -> mln_opengl_borrowed_texture_descriptor {
  return mln_opengl_borrowed_texture_descriptor{
    .size = sizeof(mln_opengl_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context = opengl_context_descriptor_default(),
    .texture = 0,
    .target = 0,
  };
}

auto webgpu_owned_texture_descriptor_default() noexcept
  -> mln_webgpu_owned_texture_descriptor {
  return mln_webgpu_owned_texture_descriptor{
    .size = sizeof(mln_webgpu_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_webgpu_context_descriptor{
      .size = sizeof(mln_webgpu_context_descriptor),
      .instance = nullptr,
      .device = nullptr,
      .queue = nullptr,
    },
  };
}

auto webgpu_borrowed_texture_descriptor_default() noexcept
  -> mln_webgpu_borrowed_texture_descriptor {
  return mln_webgpu_borrowed_texture_descriptor{
    .size = sizeof(mln_webgpu_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context =
      mln_webgpu_context_descriptor{
        .size = sizeof(mln_webgpu_context_descriptor),
        .instance = nullptr,
        .device = nullptr,
        .queue = nullptr,
      },
    .texture = nullptr,
    .texture_view = nullptr,
    .format = 0,
  };
}

auto opengl_surface_descriptor_default() noexcept
  -> mln_opengl_surface_descriptor {
  return mln_opengl_surface_descriptor{
    .size = sizeof(mln_opengl_surface_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = opengl_context_descriptor_default(),
    .surface = nullptr,
  };
}

auto validate_opengl_context(
  const mln_opengl_context_descriptor& context, bool require_supported_provider
) -> mln_status {
  if (context.size < sizeof(mln_opengl_context_descriptor)) {
    set_thread_error("mln_opengl_context_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (context.platform == MLN_OPENGL_CONTEXT_PLATFORM_WGL) {
    if (
      require_supported_provider && (opengl_supported_context_provider_mask() &
                                     MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL) == 0
    ) {
      set_thread_error("OpenGL WGL context provider is not supported");
      return MLN_STATUS_UNSUPPORTED;
    }
    if (context.data.wgl.size < sizeof(mln_wgl_context_descriptor)) {
      set_thread_error("mln_wgl_context_descriptor.size is too small");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (
      context.data.wgl.device_context == nullptr ||
      context.data.wgl.share_context == nullptr
    ) {
      set_thread_error("WGL device_context and share_context must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return MLN_STATUS_OK;
  }

  if (context.platform == MLN_OPENGL_CONTEXT_PLATFORM_EGL) {
    if (
      require_supported_provider && (opengl_supported_context_provider_mask() &
                                     MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL) == 0
    ) {
      set_thread_error("OpenGL EGL context provider is not supported");
      return MLN_STATUS_UNSUPPORTED;
    }
    if (context.data.egl.size < sizeof(mln_egl_context_descriptor)) {
      set_thread_error("mln_egl_context_descriptor.size is too small");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (
      context.data.egl.display == nullptr ||
      context.data.egl.config == nullptr ||
      context.data.egl.share_context == nullptr
    ) {
      set_thread_error(
        "EGL display, config, and share_context must not be null"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return MLN_STATUS_OK;
  }

  set_thread_error("OpenGL context platform is invalid");
  return MLN_STATUS_INVALID_ARGUMENT;
}

auto validate_webgpu_context(
  const mln_webgpu_context_descriptor& context, bool require_device
) -> mln_status {
  if (context.size < sizeof(mln_webgpu_context_descriptor)) {
    set_thread_error("mln_webgpu_context_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (require_device && context.device == nullptr) {
    set_thread_error("WebGPU device must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core

struct mln_feature_query_result {
  std::vector<std::unique_ptr<mln::core::OwnedQueriedFeatureDescriptor>>
    features;
};

struct mln_feature_extension_result {
  mln_feature_extension_result_info info{};
  std::unique_ptr<mln::core::OwnedJsonDescriptor> value;
  std::vector<std::unique_ptr<mln::core::OwnedQueriedFeatureDescriptor>>
    features;
  std::vector<mln_feature> feature_roots;
};

namespace {

auto render_session_mutex() -> std::mutex& {
  static auto value = std::mutex{};
  return value;
}

auto render_sessions() -> std::unordered_map<
  mln_render_session*, std::unique_ptr<mln_render_session>>& {
  static auto value = std::unordered_map<
    mln_render_session*, std::unique_ptr<mln_render_session>>{};
  return value;
}

auto set_native_stage_error(const char* stage, const std::exception& exception)
  -> void {
  const auto message = std::string{stage} + ": " + exception.what();
  mln::core::set_thread_error(message.c_str());
}

auto feature_query_result_mutex() -> std::mutex& {
  static auto value = std::mutex{};
  return value;
}

auto feature_query_results() -> std::unordered_map<
  const mln_feature_query_result*, std::unique_ptr<mln_feature_query_result>>& {
  static auto value = std::unordered_map<
    const mln_feature_query_result*,
    std::unique_ptr<mln_feature_query_result>>{};
  return value;
}

auto feature_extension_result_mutex() -> std::mutex& {
  static auto value = std::mutex{};
  return value;
}

auto feature_extension_results() -> std::unordered_map<
  const mln_feature_extension_result*,
  std::unique_ptr<mln_feature_extension_result>>& {
  static auto value = std::unordered_map<
    const mln_feature_extension_result*,
    std::unique_ptr<mln_feature_extension_result>>{};
  return value;
}

auto has_backend(const mln_render_session* session) -> bool {
  if (session->kind == mln::core::RenderSessionKind::Surface) {
    return session->surface.backend != nullptr;
  }
  return session->texture.backend != nullptr;
}

auto validate_dimensions(
  uint32_t width, uint32_t height, double scale_factor, const char* message
) -> mln_status {
  if (
    width == 0 || height == 0 || !std::isfinite(scale_factor) ||
    scale_factor <= 0.0
  ) {
    mln::core::set_thread_error(message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto renderer_backend(mln_render_session* session)
  -> mbgl::gfx::RendererBackend* {
  if (session->kind == mln::core::RenderSessionKind::Surface) {
    return &session->surface.backend->renderer_backend();
  }
  return session->texture.backend->renderer_backend();
}

auto validate_renderer_backend(
  mln_render_session* session, mbgl::gfx::RendererBackend*& out_backend
) -> mln_status {
  if (session->renderer == nullptr) {
    mln::core::set_thread_error("render session renderer is not available");
    return MLN_STATUS_INVALID_STATE;
  }
  auto* backend = renderer_backend(session);
  if (backend == nullptr) {
    mln::core::set_thread_error(
      "render session renderer backend is not available"
    );
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_backend = backend;
  return MLN_STATUS_OK;
}

constexpr uint32_t feature_state_selector_known_fields =
  MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
  MLN_FEATURE_STATE_SELECTOR_FEATURE_ID | MLN_FEATURE_STATE_SELECTOR_STATE_KEY;

auto validate_string_view(mln_string_view string) -> bool {
  if (string.size > 0 && string.data == nullptr) {
    mln::core::set_thread_error("string data must not be null");
    return false;
  }
  return true;
}

auto validate_string_views(
  std::span<const mln_string_view> strings, const char* name
) -> bool {
  return std::ranges::all_of(strings, [name](const auto string) -> bool {
    if (!validate_string_view(string)) {
      return false;
    }
    if (string.size == 0) {
      auto message = std::string{name} + " must not contain empty strings";
      mln::core::set_thread_error(message.c_str());
      return false;
    }
    return true;
  });
}

auto string_from_view(mln_string_view string) -> std::string {
  if (string.size == 0) {
    return {};
  }
  return std::string{string.data, string.size};
}

auto string_view_from_string(const std::string& string) -> mln_string_view {
  return mln_string_view{
    .data = string.empty() ? nullptr : string.data(), .size = string.size()
  };
}

auto validate_screen_point(mln_screen_point point) -> bool {
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
    mln::core::set_thread_error("screen point coordinates must be finite");
    return false;
  }
  return true;
}

auto validate_query_result_output(mln_feature_query_result** out_result)
  -> mln_status {
  if (out_result == nullptr) {
    mln::core::set_thread_error("out_result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_result != nullptr) {
    mln::core::set_thread_error("*out_result must be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_extension_result_output(mln_feature_extension_result** out_result)
  -> mln_status {
  if (out_result == nullptr) {
    mln::core::set_thread_error("out_result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_result != nullptr) {
    mln::core::set_thread_error("*out_result must be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto make_string_vector(std::span<const mln_string_view> strings)
  -> std::vector<std::string> {
  auto result = std::vector<std::string>{};
  result.reserve(strings.size());
  for (const auto string : strings) {
    result.emplace_back(string_from_view(string));
  }
  return result;
}

auto selector_has_field(
  const mln_feature_state_selector& selector, uint32_t field
) -> bool {
  return (selector.fields & field) != 0;
}

auto validate_feature_state_selector(
  const mln_feature_state_selector* selector, bool require_feature_id
) -> mln_status {
  if (selector == nullptr) {
    mln::core::set_thread_error("feature state selector must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (selector->size < sizeof(mln_feature_state_selector)) {
    mln::core::set_thread_error("mln_feature_state_selector.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((selector->fields & ~feature_state_selector_known_fields) != 0) {
    mln::core::set_thread_error("feature state selector has unknown fields");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!validate_string_view(selector->source_id)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (selector->source_id.size == 0) {
    mln::core::set_thread_error("feature state source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    selector_has_field(*selector, MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID) &&
    !validate_string_view(selector->source_layer_id)
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    selector_has_field(*selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID) &&
    !validate_string_view(selector->feature_id)
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    selector_has_field(*selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY) &&
    !validate_string_view(selector->state_key)
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto has_feature_id =
    selector_has_field(*selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID);
  if (require_feature_id && !has_feature_id) {
    mln::core::set_thread_error("feature state selector requires feature_id");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    selector_has_field(*selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY) &&
    !has_feature_id
  ) {
    mln::core::set_thread_error(
      "feature state selector state_key requires feature_id"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto optional_selector_string(
  const mln_feature_state_selector& selector, uint32_t field,
  mln_string_view value
) -> std::optional<std::string> {
  if (!selector_has_field(selector, field)) {
    return std::nullopt;
  }
  return string_from_view(value);
}

auto feature_state_source_layer(const mln_feature_state_selector& selector)
  -> std::optional<std::string> {
  return optional_selector_string(
    selector, MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID,
    selector.source_layer_id
  );
}

auto to_rendered_query_options(
  const mln_rendered_feature_query_options* options
) -> std::optional<mbgl::RenderedQueryOptions> {
  auto layer_ids = std::optional<std::vector<std::string>>{};
  auto filter = std::optional<mbgl::style::Filter>{};
  if (options == nullptr) {
    return mbgl::RenderedQueryOptions{};
  }
  if (options->size < sizeof(mln_rendered_feature_query_options)) {
    mln::core::set_thread_error(
      "mln_rendered_feature_query_options.size is too small"
    );
    return std::nullopt;
  }
  constexpr auto known_fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  if ((options->fields & ~known_fields) != 0) {
    mln::core::set_thread_error("rendered feature query has unknown fields");
    return std::nullopt;
  }
  if ((options->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0) {
    if (options->layer_id_count > 0 && options->layer_ids == nullptr) {
      mln::core::set_thread_error("query layer IDs must not be null");
      return std::nullopt;
    }
    auto views = std::span<const mln_string_view>{
      options->layer_ids, options->layer_id_count
    };
    if (!validate_string_views(views, "query layer IDs")) {
      return std::nullopt;
    }
    layer_ids = make_string_vector(views);
  }
  if (options->filter != nullptr) {
    auto converted_filter = mln::core::to_native_style_filter(options->filter);
    if (!converted_filter) {
      return std::nullopt;
    }
    filter = std::move(*converted_filter);
  }
  return mbgl::RenderedQueryOptions{std::move(layer_ids), std::move(filter)};
}

auto to_source_query_options(const mln_source_feature_query_options* options)
  -> std::optional<mbgl::SourceQueryOptions> {
  auto source_layer_ids = std::optional<std::vector<std::string>>{};
  auto filter = std::optional<mbgl::style::Filter>{};
  if (options == nullptr) {
    return mbgl::SourceQueryOptions{};
  }
  if (options->size < sizeof(mln_source_feature_query_options)) {
    mln::core::set_thread_error(
      "mln_source_feature_query_options.size is too small"
    );
    return std::nullopt;
  }
  constexpr auto known_fields =
    MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  if ((options->fields & ~known_fields) != 0) {
    mln::core::set_thread_error("source feature query has unknown fields");
    return std::nullopt;
  }
  if (
    (options->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0
  ) {
    if (
      options->source_layer_id_count > 0 && options->source_layer_ids == nullptr
    ) {
      mln::core::set_thread_error("query source layer IDs must not be null");
      return std::nullopt;
    }
    auto views = std::span<const mln_string_view>{
      options->source_layer_ids, options->source_layer_id_count
    };
    if (!validate_string_views(views, "query source layer IDs")) {
      return std::nullopt;
    }
    source_layer_ids = make_string_vector(views);
  }
  if (options->filter != nullptr) {
    auto converted_filter = mln::core::to_native_style_filter(options->filter);
    if (!converted_filter) {
      return std::nullopt;
    }
    filter = std::move(*converted_filter);
  }
  return mbgl::SourceQueryOptions{
    std::move(source_layer_ids), std::move(filter)
  };
}

auto to_screen_line_string(
  const mln_screen_line_string& line_string, mbgl::ScreenLineString& out_line
) -> bool {
  if (line_string.point_count == 0) {
    mln::core::set_thread_error("query line string must contain points");
    return false;
  }
  if (line_string.points == nullptr) {
    mln::core::set_thread_error("query line string points must not be null");
    return false;
  }
  auto result = mbgl::ScreenLineString{};
  result.reserve(line_string.point_count);
  for (const auto point : std::span<const mln_screen_point>{
         line_string.points, line_string.point_count
       }) {
    if (!validate_screen_point(point)) {
      return false;
    }
    result.emplace_back(point.x, point.y);
  }
  out_line = std::move(result);
  return true;
}

// Normalizes a query box and intersects it with the viewport. Native tile-space
// query geometry saturates at a few tiles past a tile's own bounds, so a box
// that over-covers the viewport can degrade into an empty answer instead of the
// visible features. Clipping to the viewport is lossless for a box: the
// intersection of a screen-space box with the viewport is the same box over the
// only region that can hold rendered features. Returns nullopt when the box
// lies entirely outside the viewport, which holds no rendered features.
auto clip_screen_box_to_viewport(
  mln_screen_box box, uint32_t width, uint32_t height
) -> std::optional<mbgl::ScreenBox> {
  const auto view_width = static_cast<double>(width);
  const auto view_height = static_cast<double>(height);
  const auto min_x = std::min(box.min.x, box.max.x);
  const auto min_y = std::min(box.min.y, box.max.y);
  const auto max_x = std::max(box.min.x, box.max.x);
  const auto max_y = std::max(box.min.y, box.max.y);
  if (min_x > view_width || min_y > view_height || max_x < 0.0 || max_y < 0.0) {
    return std::nullopt;
  }
  return mbgl::ScreenBox{
    {std::max(min_x, 0.0), std::max(min_y, 0.0)},
    {std::min(max_x, view_width), std::min(max_y, view_height)}
  };
}

auto feature_identifier_type(
  const mbgl::FeatureIdentifier& identifier,
  mln::core::OwnedQueriedFeatureDescriptor& storage
) -> uint32_t {
  if (identifier.is<mbgl::NullValue>()) {
    return MLN_FEATURE_IDENTIFIER_TYPE_NULL;
  }
  if (identifier.is<uint64_t>()) {
    storage.feature.identifier.uint_value = identifier.get<uint64_t>();
    return MLN_FEATURE_IDENTIFIER_TYPE_UINT;
  }
  if (identifier.is<int64_t>()) {
    storage.feature.identifier.int_value = identifier.get<int64_t>();
    return MLN_FEATURE_IDENTIFIER_TYPE_INT;
  }
  if (identifier.is<double>()) {
    storage.feature.identifier.double_value = identifier.get<double>();
    return MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE;
  }
  storage.identifier_string = identifier.get<std::string>();
  storage.feature.identifier.string_value =
    string_view_from_string(storage.identifier_string);
  return MLN_FEATURE_IDENTIFIER_TYPE_STRING;
}

auto make_queried_feature(
  const mbgl::Feature& feature, const std::optional<std::string>& source_id
) -> std::unique_ptr<mln::core::OwnedQueriedFeatureDescriptor> {
  auto result = std::make_unique<mln::core::OwnedQueriedFeatureDescriptor>();
  result->geometry = mln::core::to_c_geometry(feature.geometry);
  result->property_keys.reserve(feature.properties.size());
  result->property_values.reserve(feature.properties.size());
  result->property_value_roots.reserve(feature.properties.size());
  result->properties.reserve(feature.properties.size());
  for (const auto& [key, value] : feature.properties) {
    result->property_keys.emplace_back(key);
    result->property_values.emplace_back(mln::core::to_c_json_value(value));
  }
  for (std::size_t index = 0; index < result->property_values.size(); ++index) {
    result->property_value_roots.emplace_back(
      result->property_values.at(index)->root
    );
    result->properties.emplace_back(
      mln_json_member{
        .key = string_view_from_string(result->property_keys.at(index)),
        .value = &result->property_value_roots.back()
      }
    );
  }

  result->feature = mln_feature{
    .size = sizeof(mln_feature),
    .geometry = &result->geometry->root,
    .properties =
      result->properties.empty() ? nullptr : result->properties.data(),
    .property_count = result->properties.size(),
    .identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_NULL,
    .identifier = {.uint_value = 0}
  };
  result->feature.identifier_type =
    feature_identifier_type(feature.id, *result);

  result->queried = mln_queried_feature{
    .size = sizeof(mln_queried_feature),
    .fields = 0,
    .feature = result->feature,
    .source_id = {},
    .source_layer_id = {},
    .state = nullptr
  };
  result->source_id =
    feature.source.empty() && source_id ? *source_id : feature.source;
  if (!result->source_id.empty()) {
    result->queried.fields |= MLN_QUERIED_FEATURE_SOURCE_ID;
    result->queried.source_id = string_view_from_string(result->source_id);
  }
  result->source_layer_id = feature.sourceLayer;
  if (!result->source_layer_id.empty()) {
    result->queried.fields |= MLN_QUERIED_FEATURE_SOURCE_LAYER_ID;
    result->queried.source_layer_id =
      string_view_from_string(result->source_layer_id);
  }
  if (!feature.state.empty()) {
    result->state = mln::core::to_c_json_value(mbgl::Value{feature.state});
    result->queried.fields |= MLN_QUERIED_FEATURE_STATE;
    result->queried.state = &result->state->root;
  }
  return result;
}

auto create_feature_query_result(
  std::vector<mbgl::Feature> features,
  const std::optional<std::string>& source_id,
  mln_feature_query_result** out_result
) -> mln_status {
  const auto output_status = validate_query_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  auto result = std::make_unique<mln_feature_query_result>();
  result->features.reserve(features.size());
  for (const auto& feature : features) {
    result->features.emplace_back(make_queried_feature(feature, source_id));
  }
  auto* handle = result.get();
  const auto lock = std::scoped_lock{feature_query_result_mutex()};
  feature_query_results().emplace(handle, std::move(result));
  *out_result = handle;
  return MLN_STATUS_OK;
}

auto find_feature_query_result_locked(const mln_feature_query_result* result)
  -> const mln_feature_query_result* {
  const auto found = feature_query_results().find(result);
  if (found == feature_query_results().end()) {
    return nullptr;
  }
  return found->second.get();
}

auto validate_non_empty_string(mln_string_view string, const char* name)
  -> bool {
  if (!validate_string_view(string)) {
    return false;
  }
  if (string.size == 0) {
    auto message = std::string{name} + " must not be empty";
    mln::core::set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto to_feature_extension_arguments(const mln_json_value* arguments)
  -> std::optional<std::optional<std::map<std::string, mbgl::Value>>> {
  if (arguments == nullptr) {
    return std::optional<std::map<std::string, mbgl::Value>>{std::nullopt};
  }
  auto converted = mln::core::to_native_json_value(arguments);
  if (!converted) {
    return std::nullopt;
  }
  const auto* object = converted->getObject();
  if (object == nullptr) {
    mln::core::set_thread_error(
      "feature extension arguments must be a JSON object"
    );
    return std::nullopt;
  }
  auto result = std::map<std::string, mbgl::Value>{};
  for (const auto& [key, value] : *object) {
    result.emplace(key, value);
  }
  return std::optional<std::map<std::string, mbgl::Value>>{std::move(result)};
}

auto create_feature_extension_value_result(
  mbgl::Value value, mln_feature_extension_result** out_result
) -> mln_status {
  const auto output_status = validate_extension_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  auto result = std::make_unique<mln_feature_extension_result>();
  result->value = mln::core::to_c_json_value(value);
  result->info = mln_feature_extension_result_info{
    .size = sizeof(mln_feature_extension_result_info),
    .type = MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE,
    .data = {.value = &result->value->root}
  };
  auto* handle = result.get();
  const auto lock = std::scoped_lock{feature_extension_result_mutex()};
  feature_extension_results().emplace(handle, std::move(result));
  *out_result = handle;
  return MLN_STATUS_OK;
}

auto create_feature_extension_collection_result(
  mbgl::FeatureCollection features, mln_feature_extension_result** out_result
) -> mln_status {
  const auto output_status = validate_extension_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  auto result = std::make_unique<mln_feature_extension_result>();
  result->features.reserve(features.size());
  result->feature_roots.reserve(features.size());
  for (const auto& feature : features) {
    result->features.emplace_back(
      make_queried_feature(mbgl::Feature{feature}, std::nullopt)
    );
  }
  for (const auto& feature : result->features) {
    result->feature_roots.emplace_back(feature->feature);
  }
  result->info = mln_feature_extension_result_info{
    .size = sizeof(mln_feature_extension_result_info),
    .type = MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION,
    .data = {
      .feature_collection = {
        .features = result->feature_roots.empty()
                      ? nullptr
                      : result->feature_roots.data(),
        .feature_count = result->feature_roots.size()
      }
    }
  };
  auto* handle = result.get();
  const auto lock = std::scoped_lock{feature_extension_result_mutex()};
  feature_extension_results().emplace(handle, std::move(result));
  *out_result = handle;
  return MLN_STATUS_OK;
}

auto create_feature_extension_result(
  mbgl::FeatureExtensionValue value, mln_feature_extension_result** out_result
) -> mln_status {
  if (value.is<mbgl::Value>()) {
    return create_feature_extension_value_result(
      std::move(value.get<mbgl::Value>()), out_result
    );
  }
  return create_feature_extension_collection_result(
    std::move(value.get<mbgl::FeatureCollection>()), out_result
  );
}

auto find_feature_extension_result_locked(
  const mln_feature_extension_result* result
) -> const mln_feature_extension_result* {
  const auto found = feature_extension_results().find(result);
  if (found == feature_extension_results().end()) {
    return nullptr;
  }
  return found->second.get();
}

// The map's scale factor is fixed at creation and still selects sprites,
// glyphs, and raster tiles, while the session's scale factor drives geometry
// and shaders. A mismatch renders correctly sized geometry against imagery
// chosen for a different density, so warn instead of failing the call.
auto warn_on_scale_factor_mismatch(mln_map* map, double scale_factor) -> void {
  constexpr auto tolerance = 1e-6;
  const auto creation_scale_factor = mln::core::map_scale_factor(map);
  if (std::abs(creation_scale_factor - scale_factor) <= tolerance) {
    return;
  }
  mbgl::Log::Warning(
    mbgl::Event::Render,
    "render target scale_factor " + mbgl::util::toString(scale_factor) +
      " differs from the map scale_factor " +
      mbgl::util::toString(creation_scale_factor) +
      "; the map value is fixed at creation and still selects sprites, glyphs, "
      "and raster tiles, so styled imagery will not match the rendered "
      "geometry. Create the map with the scale factor you intend to render at."
  );
}

}  // namespace

namespace mln::core {

auto register_render_session(
  mln_render_session* handle, std::unique_ptr<mln_render_session> session
) -> void {
  const auto lock = std::scoped_lock{render_session_mutex()};
  render_sessions().emplace(handle, std::move(session));
}

void RenderSessionScheduler::schedule(std::function<void()>&& task) {
  const auto lock = std::scoped_lock{mutex_};
  queue_.push_back(std::move(task));
}

void RenderSessionScheduler::schedule(
  const mbgl::util::SimpleIdentity, std::function<void()>&& task
) {
  schedule(std::move(task));
}

auto RenderSessionScheduler::drain() -> void {
  {
    const auto lock = std::scoped_lock{mutex_};
    if (draining_) {
      // A task re-entered drain(). The loop below still owns the queue.
      return;
    }
    draining_ = true;
  }
  // Clear the flag on every exit, including a throwing task. Leaving it set
  // would silently stop this session from ever delivering results again.
  const auto clear_draining = DrainGuard{*this};
  while (true) {
    auto batch = std::vector<std::function<void()>>{};
    {
      const auto lock = std::scoped_lock{mutex_};
      if (queue_.empty()) {
        return;
      }
      batch.swap(queue_);
    }
    // Run outside the lock: a task may schedule more work.
    for (auto& task : batch) {
      if (task) {
        task();
      }
    }
  }
}

RenderSessionScheduler::DrainGuard::~DrainGuard() {
  const auto lock = std::scoped_lock{scheduler_.mutex_};
  scheduler_.draining_ = false;
}

auto RenderSessionScheduler::discard() -> void {
  auto batch = std::vector<std::function<void()>>{};
  const auto lock = std::scoped_lock{mutex_};
  batch.swap(queue_);
}

auto validate_render_session(mln_render_session* session) -> mln_status {
  if (session == nullptr) {
    set_thread_error("render session must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto lock = std::scoped_lock{render_session_mutex()};
  if (!render_sessions().contains(session)) {
    set_thread_error("render session is not a live handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (session->owner_thread != std::this_thread::get_id()) {
    set_thread_error(
      "render session call must be made on the thread that attached it"
    );
    return MLN_STATUS_WRONG_THREAD;
  }
  return MLN_STATUS_OK;
}

auto validate_live_attached_render_session(mln_render_session* session)
  -> mln_status {
  const auto status = validate_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!session->attached || !has_backend(session)) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto erase_render_session(mln_render_session* session)
  -> std::unique_ptr<mln_render_session> {
  const auto lock = std::scoped_lock{render_session_mutex()};
  auto found = render_sessions().find(session);
  if (found == render_sessions().end()) {
    return nullptr;
  }
  auto owned = std::move(found->second);
  render_sessions().erase(found);
  return owned;
}

auto attach_render_session(
  std::unique_ptr<mln_render_session> session, mln_render_session** out_session,
  RenderSessionKind kind, RenderSessionAttachMessages messages
) -> mln_status {
  if (session == nullptr) {
    set_thread_error(messages.null_session);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto output_status = validate_attach_output(
    out_session, messages.null_output, messages.non_null_output
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  // The attaching thread owns the session for its whole lifetime. The session
  // records its own thread rather than inheriting the map's, which is what lets
  // a host attach where its graphics context is current while the map stays on
  // the runtime loop thread.
  session->owner_thread = std::this_thread::get_id();

  auto* map = session->map;
  auto* handle = session.get();
  const auto attach_status = map_attach_render_target_session(map, handle);
  if (attach_status != MLN_STATUS_OK) {
    return attach_status;
  }
  try {
    // Set before priming: renderer_backend() dispatches on kind.
    session->kind = kind;
    // Create the backend's graphics context here, on the thread that will drive
    // the session and where the host's context is current. WGL resolves
    // wglCreateContextAttribsARB through wglGetProcAddress, which needs a
    // current context, and shares against the host context by checking whether
    // it is current on *this* thread. This is why attach must be called on the
    // render loop thread rather than the map's: a graphics context cannot be
    // current on two threads at once.
    // Vulkan and Metal activate() are no-ops, so this costs them nothing.
    if (auto* backend = renderer_backend(handle); backend != nullptr) {
      const auto prime = mbgl::gfx::BackendScope{*backend};
    }

    // Only now that the graphics setup has succeeded. The map applies this on
    // its own thread, so a size queued before a throwing prime would still land
    // and resize the map to an extent whose attach failed.
    const auto size_status =
      map_post_set_size(map, session->width, session->height);
    if (size_status != MLN_STATUS_OK) {
      static_cast<void>(map_detach_render_target_session(map, handle));
      return size_status;
    }
    warn_on_scale_factor_mismatch(map, session->scale_factor);

    register_render_session(handle, std::move(session));
  } catch (...) {
    static_cast<void>(map_detach_render_target_session(map, handle));
    throw;
  }

  *out_session = handle;
  return MLN_STATUS_OK;
}

auto render_session_resize(
  mln_render_session* session, uint32_t width, uint32_t height,
  double scale_factor
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto dimensions_status = validate_dimensions(
    width, height, scale_factor,
    session->kind == RenderSessionKind::Surface
      ? "surface dimensions and scale_factor must be positive"
      : "texture dimensions and scale_factor must be positive"
  );
  if (dimensions_status != MLN_STATUS_OK) {
    return dimensions_status;
  }
  if (
    session->kind == RenderSessionKind::Texture && session->texture.acquired
  ) {
    set_thread_error("cannot resize while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    session->kind == RenderSessionKind::Texture &&
    session->texture.mode == TextureSessionMode::Borrowed
  ) {
    set_thread_error(
      "borrowed texture sessions cannot be resized; attach a new target"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto physical_status = validate_physical_size(
    width, height, scale_factor,
    session->kind == RenderSessionKind::Surface
      ? "scaled surface dimensions are too large"
      : "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  const auto physical_width = physical_dimension(width, scale_factor);
  const auto physical_height = physical_dimension(height, scale_factor);
  if (session->kind == RenderSessionKind::Surface) {
    session->surface.backend->resize(physical_width, physical_height);
  } else {
    session->texture.backend->headless_backend().setSize(
      mbgl::Size{physical_width, physical_height}
    );
    session->texture.rendered_native_texture = nullptr;
    session->texture.acquired_native_texture = nullptr;
    session->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  }
  const auto size_status = map_post_set_size(session->map, width, height);
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  warn_on_scale_factor_mismatch(session->map, scale_factor);
  session->renderer.reset();
  session->rendered_generation = 0;
  session->width = width;
  session->height = height;
  session->physical_width = physical_width;
  session->physical_height = physical_height;
  session->scale_factor = scale_factor;
  ++session->generation;
  return MLN_STATUS_OK;
}

auto render_session_render_update(
  mln_render_session* session, bool* out_rendered
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_rendered == nullptr) {
    set_thread_error("out_rendered must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_rendered = false;
  if (
    session->kind == RenderSessionKind::Texture && session->texture.acquired
  ) {
    set_thread_error("cannot render while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }

  auto* backend = renderer_backend(session);
  if (backend == nullptr) {
    set_thread_error("render session renderer backend is not available");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  // Deliver tile and resource results first: destroying the tiles they retire
  // is what enqueues the GPU-release work that map_run_render_jobs() drains.
  //
  // Before the early returns below, not after. When this session owns the
  // thread's scheduler, this queue is the only place those results land, and it
  // drains nowhere else. Returning without draining would strand them whenever
  // there is no update to render, which is exactly the window a resize opens.
  session->scheduler.drain();
  map_run_render_jobs(session->map);

  auto update = map_latest_update(session->map);
  if (!update) {
    return MLN_STATUS_OK;
  }

  // The map applies its logical size on its own thread, so a resize leaves this
  // session holding an update built for the previous extent while the render
  // target has already changed. Rendering it anyway would take the projection
  // from the update's logical size but the viewport and scissor from the
  // backend's physical size, producing a visibly stretched frame. Wait for the
  // update that matches instead. This cannot stall: Transform::resize
  // publishes a new update unless the size is already what we want.
  if (
    update->transformState.getSize() !=
    mbgl::Size{session->width, session->height}
  ) {
    return MLN_STATUS_OK;
  }

  if (session->kind == RenderSessionKind::Texture) {
    session->texture.backend->prepare_render_resources();
  }
  if (session->renderer == nullptr) {
    try {
      session->renderer = std::make_unique<mbgl::Renderer>(
        *backend, static_cast<float>(session->scale_factor)
      );
      session->renderer->setObserver(map_renderer_observer(session->map));
    } catch (const std::exception& exception) {
      set_native_stage_error("creating renderer", exception);
      return MLN_STATUS_NATIVE_ERROR;
    }
  }

  try {
    session->renderer->render(update);
  } catch (const std::exception& exception) {
    set_native_stage_error("rendering update", exception);
    return MLN_STATUS_NATIVE_ERROR;
  }
  // Absorb results that landed from worker threads during the render, so they
  // do not wait a whole frame.
  session->scheduler.drain();
  if (session->kind == RenderSessionKind::Texture) {
    auto frame_rendered = true;
    const auto after_status =
      session->texture.backend->after_render(*session, frame_rendered);
    if (after_status != MLN_STATUS_OK) {
      return after_status;
    }
    if (!frame_rendered) {
      return MLN_STATUS_OK;
    }
  }
  session->rendered_generation = session->generation;
  *out_rendered = true;
  return MLN_STATUS_OK;
}

auto render_session_detach(mln_render_session* session) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    session->kind == RenderSessionKind::Texture && session->texture.acquired
  ) {
    set_thread_error("cannot detach while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }

  // Tear the renderer down before releasing the map's slot. The renderer holds
  // the map's forwarding observer, which the map's frontend owns, and queued
  // work can reach it while it drains. Releasing the slot first would let the
  // map owner thread see no attached session and destroy the map — freeing that
  // observer underneath the drain and reset below.
  {
    auto current = ScopedCurrentScheduler{session->scheduler};
    // Let queued results land while the renderer they target still exists.
    session->scheduler.drain();
    session->renderer.reset();
    session->surface.backend.reset();
    session->texture.backend.reset();
    // Anything enqueued during teardown is a mailbox receive or a bindOnce
    // continuation whose target is already gone, so running it would be a
    // no-op. Drop it rather than execute arbitrary work mid-teardown.
    session->scheduler.discard();
  }

  const auto detach_status =
    map_detach_render_target_session(session->map, session);
  if (detach_status != MLN_STATUS_OK) {
    return detach_status;
  }
  session->attached = false;
  session->rendered_generation = 0;
  session->texture.rendered_native_texture = nullptr;
  session->texture.acquired_native_texture = nullptr;
  session->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  ++session->generation;
  return MLN_STATUS_OK;
}

auto render_session_destroy(mln_render_session* session) -> mln_status {
  const auto status = validate_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    session->kind == RenderSessionKind::Texture && session->texture.acquired
  ) {
    set_thread_error("cannot destroy while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (session->attached) {
    const auto detach_status = render_session_detach(session);
    if (detach_status != MLN_STATUS_OK) {
      return detach_status;
    }
  }
  auto owned_session = erase_render_session(session);
  owned_session.reset();
  return MLN_STATUS_OK;
}

auto render_session_reduce_memory_use(mln_render_session* session)
  -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  session->renderer->reduceMemoryUse();
  return MLN_STATUS_OK;
}

auto render_session_clear_data(mln_render_session* session) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  session->renderer->clearData();
  return MLN_STATUS_OK;
}

auto render_session_dump_debug_logs(mln_render_session* session) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  session->renderer->dumpDebugLogs();
  return MLN_STATUS_OK;
}

auto render_session_set_feature_state(
  mln_render_session* session, const mln_feature_state_selector* selector,
  const mln_json_value* state
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto native_state = to_native_json_value(state);
  if (!native_state) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto* state_object = native_state->getObject();
  if (state_object == nullptr) {
    set_thread_error("feature state value must be a JSON object");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  session->renderer->setFeatureState(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    string_from_view(selector->feature_id), *state_object
  );
  static_cast<void>(map_post_trigger_repaint(session->map));
  return MLN_STATUS_OK;
}

auto render_session_get_feature_state(
  mln_render_session* session, const mln_feature_state_selector* selector,
  mln_json_snapshot** out_state
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto state = mbgl::FeatureState{};
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  session->renderer->getFeatureState(
    state, string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    string_from_view(selector->feature_id)
  );
  return json_snapshot_create(mbgl::Value{std::move(state)}, out_state);
}

auto render_session_remove_feature_state(
  mln_render_session* session, const mln_feature_state_selector* selector
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, false);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  session->renderer->removeFeatureState(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, selector->feature_id
    ),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY, selector->state_key
    )
  );
  static_cast<void>(map_post_trigger_repaint(session->map));
  return MLN_STATUS_OK;
}

auto render_session_query_rendered_features(
  mln_render_session* session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_feature_query_result** out_result
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto output_status = validate_query_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  if (geometry == nullptr) {
    set_thread_error("rendered query geometry must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (geometry->size < sizeof(mln_rendered_query_geometry)) {
    set_thread_error("mln_rendered_query_geometry.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto native_options = to_rendered_query_options(options);
  if (!native_options) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto line_string = mbgl::ScreenLineString{};
  auto clipped_box = std::optional<mbgl::ScreenBox>{};
  switch (geometry->type) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      if (!validate_screen_point(geometry->data.point)) {
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX: {
      if (
        !validate_screen_point(geometry->data.box.min) ||
        !validate_screen_point(geometry->data.box.max)
      ) {
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      clipped_box = clip_screen_box_to_viewport(
        geometry->data.box, session->width, session->height
      );
      break;
    }
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING: {
      if (!to_screen_line_string(geometry->data.line_string, line_string)) {
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      break;
    }
    default:
      set_thread_error("rendered query geometry type is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }

  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  auto features = std::vector<mbgl::Feature>{};
  switch (geometry->type) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      features = session->renderer->queryRenderedFeatures(
        mbgl::ScreenCoordinate{geometry->data.point.x, geometry->data.point.y},
        *native_options
      );
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX:
      if (clipped_box) {
        features = session->renderer->queryRenderedFeatures(
          *clipped_box, *native_options
        );
      }
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING:
      features =
        session->renderer->queryRenderedFeatures(line_string, *native_options);
      break;
    default:
      set_thread_error("rendered query geometry type is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
  return create_feature_query_result(
    std::move(features), std::nullopt, out_result
  );
}

auto render_session_query_source_features(
  mln_render_session* session, mln_string_view source_id,
  const mln_source_feature_query_options* options,
  mln_feature_query_result** out_result
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto output_status = validate_query_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  if (!validate_string_view(source_id)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto native_options = to_source_query_options(options);
  if (!native_options) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto native_source_id = string_from_view(source_id);
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  auto features =
    session->renderer->querySourceFeatures(native_source_id, *native_options);
  return create_feature_query_result(
    std::move(features), native_source_id, out_result
  );
}

auto render_session_query_feature_extensions(
  mln_render_session* session, mln_string_view source_id,
  const mln_feature* feature, mln_string_view extension,
  mln_string_view extension_field, const mln_json_value* arguments,
  mln_feature_extension_result** out_result
) -> mln_status {
  const auto status = validate_live_attached_render_session(session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto output_status = validate_extension_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  if (
    !validate_non_empty_string(source_id, "source_id") ||
    !validate_non_empty_string(extension, "extension") ||
    !validate_non_empty_string(extension_field, "extension_field")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto native_feature = to_native_feature(feature);
  if (!native_feature) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto native_arguments = to_feature_extension_arguments(arguments);
  if (!native_arguments) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(session, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto query_feature = mbgl::Feature{std::move(*native_feature)};
  auto current = ScopedCurrentScheduler{session->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  auto result = session->renderer->queryFeatureExtensions(
    string_from_view(source_id), query_feature, string_from_view(extension),
    string_from_view(extension_field), std::move(*native_arguments)
  );
  return create_feature_extension_result(std::move(result), out_result);
}

auto feature_query_result_count(
  const mln_feature_query_result* result, std::size_t* out_count
) -> mln_status {
  if (result == nullptr) {
    set_thread_error("feature query result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_count == nullptr) {
    set_thread_error("out_count must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto lock = std::scoped_lock{feature_query_result_mutex()};
  const auto* live_result = find_feature_query_result_locked(result);
  if (live_result == nullptr) {
    set_thread_error("feature query result is not a live handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_count = live_result->features.size();
  return MLN_STATUS_OK;
}

auto feature_query_result_get(
  const mln_feature_query_result* result, std::size_t index,
  mln_queried_feature* out_feature
) -> mln_status {
  if (result == nullptr) {
    set_thread_error("feature query result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_feature == nullptr) {
    set_thread_error("out_feature must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_feature->size < sizeof(mln_queried_feature)) {
    set_thread_error("mln_queried_feature.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto lock = std::scoped_lock{feature_query_result_mutex()};
  const auto* live_result = find_feature_query_result_locked(result);
  if (live_result == nullptr) {
    set_thread_error("feature query result is not a live handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (index >= live_result->features.size()) {
    set_thread_error("index is out of range");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_feature = live_result->features.at(index)->queried;
  return MLN_STATUS_OK;
}

auto feature_query_result_destroy(mln_feature_query_result* result) -> void {
  if (result == nullptr) {
    return;
  }
  const auto lock = std::scoped_lock{feature_query_result_mutex()};
  feature_query_results().erase(result);
}

auto feature_extension_result_get(
  const mln_feature_extension_result* result,
  mln_feature_extension_result_info* out_info
) -> mln_status {
  if (result == nullptr) {
    set_thread_error("feature extension result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_info == nullptr) {
    set_thread_error("out_info must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_info->size < sizeof(mln_feature_extension_result_info)) {
    set_thread_error("mln_feature_extension_result_info.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto lock = std::scoped_lock{feature_extension_result_mutex()};
  const auto* live_result = find_feature_extension_result_locked(result);
  if (live_result == nullptr) {
    set_thread_error("feature extension result is not a live handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_info = live_result->info;
  return MLN_STATUS_OK;
}

auto feature_extension_result_destroy(mln_feature_extension_result* result)
  -> void {
  if (result == nullptr) {
    return;
  }
  const auto lock = std::scoped_lock{feature_extension_result_mutex()};
  feature_extension_results().erase(result);
}

}  // namespace mln::core
