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

#include "bytes/buffer.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "style/style_value.hpp"

namespace mln::core {

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
#elif defined(MLN_RENDER_BACKEND_OPENGL) && \
  defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  return MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL;
#else
  return 0;
#endif
}

auto opengl_context_descriptor_default() noexcept
  -> mln_opengl_context_descriptor {
  auto result = mln_opengl_context_descriptor{
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED,
    .ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED,
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
    .client_api = MLN_OPENGL_CLIENT_API_UNSPECIFIED,
    .get_proc_address = nullptr,
  };
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  result.platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
  result.data.webgl = mln_webgl_context_descriptor{
    .size = sizeof(mln_webgl_context_descriptor),
    .context = 0,
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

auto webgpu_surface_descriptor_default() noexcept
  -> mln_webgpu_surface_descriptor {
  return mln_webgpu_surface_descriptor{
    .size = sizeof(mln_webgpu_surface_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context =
      mln_webgpu_context_descriptor{
        .size = sizeof(mln_webgpu_context_descriptor),
        .instance = nullptr,
        .device = nullptr,
        .queue = nullptr,
      },
    .surface = nullptr,
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

// The instance and queue are optional: no session kind needs the instance, and
// a null queue means the device's default queue.
auto validate_webgpu_context(const mln_webgpu_context_descriptor& context)
  -> mln_status {
  if (context.size < sizeof(mln_webgpu_context_descriptor)) {
    set_thread_error("mln_webgpu_context_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (context.device == nullptr) {
    set_thread_error("WebGPU device must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_opengl_context(
  const mln_opengl_context_descriptor& context, bool require_supported_provider
) -> mln_status {
  if (context.size < sizeof(mln_opengl_context_descriptor)) {
    set_thread_error("mln_opengl_context_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    context.ownership != MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED &&
    context.ownership != MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED
  ) {
    set_thread_error("mln_opengl_context_descriptor.ownership is unknown");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto dedicated =
    context.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;

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
    if (context.data.wgl.device_context == nullptr) {
      set_thread_error("WGL device_context must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (dedicated) {
      if (context.data.wgl.share_context != nullptr) {
        set_thread_error(
          "a dedicated WGL context joins no share group, so share_context must "
          "be null"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      return MLN_STATUS_OK;
    }
    if (context.data.wgl.share_context == nullptr) {
      set_thread_error("WGL share_context must not be null");
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
      context.data.egl.display == nullptr || context.data.egl.config == nullptr
    ) {
      set_thread_error("EGL display and config must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (dedicated) {
      if (context.data.egl.share_context != nullptr) {
        set_thread_error(
          "a dedicated EGL context joins no share group, so share_context must "
          "be null"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (
        context.data.egl.client_api != MLN_OPENGL_CLIENT_API_GL &&
        context.data.egl.client_api != MLN_OPENGL_CLIENT_API_GLES
      ) {
        set_thread_error(
          "a dedicated EGL context has no share context to take its client API "
          "from, so client_api must name one"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      return MLN_STATUS_OK;
    }
    if (context.data.egl.share_context == nullptr) {
      set_thread_error("EGL share_context must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return MLN_STATUS_OK;
  }

  if (context.platform == MLN_OPENGL_CONTEXT_PLATFORM_WEBGL) {
    if (
      require_supported_provider &&
      (opengl_supported_context_provider_mask() &
       MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL) == 0
    ) {
      set_thread_error("OpenGL WebGL context provider is not supported");
      return MLN_STATUS_UNSUPPORTED;
    }
    if (context.data.webgl.size < sizeof(mln_webgl_context_descriptor)) {
      set_thread_error("mln_webgl_context_descriptor.size is too small");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (dedicated) {
      // A browser session renders through the context the host created and
      // still owns, so there is nothing for the session to take over.
      set_thread_error("a WebGL context is always shared with its host");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (context.data.webgl.context <= 0) {
      set_thread_error("WebGL context handle must be positive");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return MLN_STATUS_OK;
  }

  set_thread_error("OpenGL context platform is invalid");
  return MLN_STATUS_INVALID_ARGUMENT;
}

auto vulkan_context_matches(
  const mln_vulkan_context_descriptor& lhs,
  const mln_vulkan_context_descriptor& rhs
) -> bool {
  return lhs.instance == rhs.instance &&
         lhs.physical_device == rhs.physical_device &&
         lhs.device == rhs.device && lhs.graphics_queue == rhs.graphics_queue &&
         lhs.graphics_queue_family_index == rhs.graphics_queue_family_index;
}

auto opengl_context_matches(
  const mln_opengl_context_descriptor& lhs,
  const mln_opengl_context_descriptor& rhs, OpenGLContextMatch strictness
) -> bool {
  if (lhs.platform != rhs.platform) {
    return false;
  }
  switch (lhs.platform) {
    case MLN_OPENGL_CONTEXT_PLATFORM_WGL:
      if (lhs.ownership != rhs.ownership) {
        return false;
      }
      // A dedicated context names no share context, so the device context it
      // was created from is what identifies it under either strictness.
      if (
        (strictness == OpenGLContextMatch::Exact ||
         lhs.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED) &&
        lhs.data.wgl.device_context != rhs.data.wgl.device_context
      ) {
        return false;
      }
      return lhs.data.wgl.share_context == rhs.data.wgl.share_context;
    case MLN_OPENGL_CONTEXT_PLATFORM_EGL:
      // EGL names its drawable in the target rather than in the context, so
      // both strictnesses ask for the same three handles.
      // A dedicated context names no share context, so its identity rests on
      // the display, the config, and the API it was created for. A shared
      // context takes its API from the share context, which leaves client_api
      // ignored there and so outside its identity.
      if (lhs.ownership != rhs.ownership) {
        return false;
      }
      if (
        lhs.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED &&
        lhs.data.egl.client_api != rhs.data.egl.client_api
      ) {
        return false;
      }
      return lhs.data.egl.display == rhs.data.egl.display &&
             lhs.data.egl.config == rhs.data.egl.config &&
             lhs.data.egl.share_context == rhs.data.egl.share_context;
    case MLN_OPENGL_CONTEXT_PLATFORM_WEBGL:
      // A WebGL context carries its own drawable, so the handle is all there is
      // to compare under either strictness.
      return lhs.data.webgl.context == rhs.data.webgl.context;
    case MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED:
      break;
  }
  return false;
}

}  // namespace mln::core

namespace mln::core {

template <>
struct HandleTraits<mln_render_session_object> {
  static constexpr auto kind = HandleKind::RenderSession;
  static constexpr auto leasable = false;
};

struct QueriedFeatureRecord {
  std::string feature;
  std::string source_id;
  std::string source_layer_id;
  std::string state;
  uint32_t fields = 0;
};

struct QueriedFeatureListObject {
  std::vector<QueriedFeatureRecord> features;
};

template <>
struct HandleTraits<QueriedFeatureListObject> {
  static constexpr auto kind = HandleKind::QueriedFeatureList;
  static constexpr auto leasable = false;
};

}  // namespace mln::core

namespace {

auto set_native_stage_error(const char* stage, const std::exception& exception)
  -> void {
  const auto message = std::string{stage} + ": " + exception.what();
  mln::core::set_thread_error(message.c_str());
}

auto has_backend(const mln_render_session_object* session) -> bool {
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

auto renderer_backend(mln_render_session_object* session)
  -> mbgl::gfx::RendererBackend* {
  if (session->kind == mln::core::RenderSessionKind::Surface) {
    return &session->surface.backend->renderer_backend();
  }
  return session->texture.backend->renderer_backend();
}

auto validate_renderer_backend(
  mln_render_session_object* session, mbgl::gfx::RendererBackend*& out_backend
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

auto validate_string_view(mln_buffer_view string) -> bool {
  if (string.size > 0 && string.data == nullptr) {
    mln::core::set_thread_error("string data must not be null");
    return false;
  }
  return true;
}

auto validate_string_views(
  std::span<const mln_buffer_view> strings, const char* name
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

auto string_from_view(mln_buffer_view string) -> std::string {
  if (string.size == 0) {
    return {};
  }
  return std::string{static_cast<const char*>(string.data), string.size};
}

auto validate_screen_point(mln_screen_point point) -> bool {
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
    mln::core::set_thread_error("screen point coordinates must be finite");
    return false;
  }
  return true;
}

template <typename Handle>
auto validate_result_output(Handle* out_result) -> mln_status {
  if (out_result == nullptr) {
    mln::core::set_thread_error("out_result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_result != MLN_HANDLE_NULL) {
    mln::core::set_thread_error("*out_result must be the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto make_string_vector(std::span<const mln_buffer_view> strings)
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
  mln_buffer_view value
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
    auto views = std::span<const mln_buffer_view>{
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
    auto views = std::span<const mln_buffer_view>{
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
// query geometry saturates a few tiles past a tile's own bounds, so a box that
// over-covers the viewport degrades into an empty answer. Returns nullopt when
// the box lies entirely outside the viewport.
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

auto serialize_geojson_feature(const mbgl::Feature& feature) -> std::string {
  return mln::core::serialize_geojson(
    mbgl::GeoJSON{static_cast<const mbgl::GeoJSONFeature&>(feature)}
  );
}

auto create_feature_query_result(
  std::vector<mbgl::Feature> features,
  const std::optional<std::string>& source_id,
  mln_queried_feature_list* out_result
) -> mln_status {
  const auto output_status = validate_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  auto list = std::make_shared<mln::core::QueriedFeatureListObject>();
  list->features.reserve(features.size());
  for (const auto& feature : features) {
    auto record = mln::core::QueriedFeatureRecord{};
    record.feature = serialize_geojson_feature(feature);
    const auto effective_source_id =
      feature.source.empty() && source_id ? *source_id : feature.source;
    if (!effective_source_id.empty()) {
      record.source_id = effective_source_id;
      record.fields |= MLN_QUERIED_FEATURE_SOURCE_ID;
    }
    if (!feature.sourceLayer.empty()) {
      record.source_layer_id = feature.sourceLayer;
      record.fields |= MLN_QUERIED_FEATURE_SOURCE_LAYER_ID;
    }
    if (!feature.state.empty()) {
      record.state =
        mln::core::serialize_json_value(mbgl::Value{feature.state});
      record.fields |= MLN_QUERIED_FEATURE_STATE;
    }
    list->features.push_back(std::move(record));
  }
  *out_result =
    mln::core::handle_table<mln::core::QueriedFeatureListObject>().insert(
      std::move(list)
    );
  return MLN_STATUS_OK;
}

auto validate_non_empty_string(mln_buffer_view string, const char* name)
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

auto to_feature_extension_arguments(const mln_buffer_view* arguments)
  -> std::optional<std::optional<std::map<std::string, mbgl::Value>>> {
  if (arguments == nullptr) {
    return std::optional<std::map<std::string, mbgl::Value>>{std::nullopt};
  }
  auto converted = mln::core::to_native_json_value(*arguments);
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

auto create_feature_extension_result(
  mbgl::FeatureExtensionValue value, mln_buffer* out_result
) -> mln_status {
  const auto output_status = validate_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  if (value.is<mbgl::Value>()) {
    return mln::core::create_buffer(
      mln::core::serialize_json_value(value.get<mbgl::Value>()), out_result
    );
  }
  return mln::core::create_buffer(
    mln::core::serialize_feature_collection(
      value.get<mbgl::FeatureCollection>()
    ),
    out_result
  );
}

// The map's scale factor is fixed at creation and selects sprites, glyphs, and
// raster tiles; the session's drives geometry and shaders. A mismatch renders
// correctly sized geometry against imagery chosen for a different density.
auto warn_on_scale_factor_mismatch(mln_map map, double scale_factor) -> void {
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

auto register_render_session(std::shared_ptr<mln_render_session_object> session)
  -> mln_render_session {
  return handle_table<mln_render_session_object>().insert(std::move(session));
}

void RenderSessionScheduler::schedule(std::function<void()>&& task) {
  auto request_repaint = std::function<void()>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    const auto was_empty = queue_.empty();
    queue_.push_back(std::move(task));
    if (was_empty && !draining_) {
      request_repaint = repaint_request_;
    }
  }
  if (request_repaint) {
    request_repaint();
  }
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
  const auto clear_draining = DrainGuard{*this};
  while (true) {
    auto batch = std::vector<std::function<void()>>{};
    {
      const auto lock = std::scoped_lock{mutex_};
      if (queue_.empty()) {
        draining_ = false;
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
  auto request_repaint = std::function<void()>{};
  {
    const auto lock = std::scoped_lock{scheduler_.mutex_};
    if (scheduler_.draining_) {
      scheduler_.draining_ = false;
      if (!scheduler_.queue_.empty()) {
        request_repaint = scheduler_.repaint_request_;
      }
    }
  }
  if (request_repaint) {
    request_repaint();
  }
}

auto RenderSessionScheduler::discard() -> void {
  auto batch = std::vector<std::function<void()>>{};
  const auto lock = std::scoped_lock{mutex_};
  batch.swap(queue_);
}

auto RenderSessionScheduler::set_repaint_request(
  std::function<void()> repaint_request
) -> void {
  const auto lock = std::scoped_lock{mutex_};
  repaint_request_ = std::move(repaint_request);
}

// Only the attaching thread destroys a session, so the borrowed object stays
// alive for as long as the calling thread can use it.
auto validate_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status {
  out_session = handle_table<mln_render_session_object>().resolve(session);
  if (out_session == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_session->owner_thread != std::this_thread::get_id()) {
    set_thread_error(
      "render session call must be made on the thread that attached it"
    );
    return MLN_STATUS_WRONG_THREAD;
  }
  return MLN_STATUS_OK;
}

auto validate_live_attached_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status {
  const auto status = validate_render_session(session, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!out_session->attached || !has_backend(out_session)) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto erase_render_session(mln_render_session session)
  -> std::shared_ptr<mln_render_session_object> {
  return handle_table<mln_render_session_object>().remove(session);
}

auto attach_render_session(
  std::shared_ptr<mln_render_session_object> session,
  mln_render_session* out_session, RenderSessionKind kind,
  RenderSessionAttachMessages messages
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

  // The attaching thread owns the session for its whole lifetime, and need not
  // be the map's thread.
  session->owner_thread = std::this_thread::get_id();

  const auto map = session->map;
  auto* handle = session.get();
  const auto attach_status = map_attach_render_target_session(map, handle);
  if (attach_status != MLN_STATUS_OK) {
    return attach_status;
  }
  try {
    session->scheduler.set_repaint_request([map]() {
      static_cast<void>(map_post_trigger_repaint(map));
    });
    // Set before priming: renderer_backend() dispatches on kind.
    session->kind = kind;
    // Create the backend's graphics context on the thread that will drive the
    // session, where the host's context is current: WGL resolves
    // wglCreateContextAttribsARB through wglGetProcAddress and shares against
    // the host context only if it is current on this thread.
    if (auto* backend = renderer_backend(handle); backend != nullptr) {
      const auto prime = mbgl::gfx::BackendScope{*backend};
    }

    // After the graphics setup succeeds: the map applies this on its own
    // thread, so a size queued before a throwing prime would still land.
    const auto size_status =
      map_post_set_size(map, session->width, session->height);
    if (size_status != MLN_STATUS_OK) {
      session->scheduler.set_repaint_request({});
      static_cast<void>(map_detach_render_target_session(map, handle));
      return size_status;
    }
    warn_on_scale_factor_mismatch(map, session->scale_factor);

    *out_session = register_render_session(std::move(session));
  } catch (...) {
    session->scheduler.set_repaint_request({});
    static_cast<void>(map_detach_render_target_session(map, handle));
    throw;
  }

  return MLN_STATUS_OK;
}

auto render_session_resize(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto dimensions_status = validate_dimensions(
    width, height, scale_factor,
    live->kind == RenderSessionKind::Surface
      ? "surface dimensions and scale_factor must be positive"
      : "texture dimensions and scale_factor must be positive"
  );
  if (dimensions_status != MLN_STATUS_OK) {
    return dimensions_status;
  }
  if (live->kind == RenderSessionKind::Texture && live->texture.acquired) {
    set_thread_error("cannot resize while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    live->kind == RenderSessionKind::Texture &&
    live->texture.mode == TextureSessionMode::Borrowed
  ) {
    set_thread_error(
      "a caller-owned texture is sized by its owner; hand over a replacement "
      "with the borrowed-texture set_target function for this backend"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto physical_status = validate_physical_size(
    width, height, scale_factor,
    live->kind == RenderSessionKind::Surface
      ? "scaled surface dimensions are too large"
      : "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  const auto physical_width = physical_dimension(width, scale_factor);
  const auto physical_height = physical_dimension(height, scale_factor);
  if (live->kind == RenderSessionKind::Surface) {
    live->surface.backend->resize(physical_width, physical_height);
  } else {
    live->texture.backend->resize(mbgl::Size{physical_width, physical_height});
    live->texture.rendered_native_texture = nullptr;
    live->texture.acquired_native_texture = nullptr;
    live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  }
  const auto size_status = map_post_set_size(live->map, width, height);
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  warn_on_scale_factor_mismatch(live->map, scale_factor);
  // Keep the renderer across a resize, which carries the tile pyramid, atlases,
  // symbol placement, and feature state. Pixel ratio is the exception: it is
  // fixed when a Renderer is constructed and baked into its shaders.
  if (scale_factor != live->scale_factor) {
    live->renderer.reset();
  }
  live->rendered_generation = 0;
  live->width = width;
  live->height = height;
  live->physical_width = physical_width;
  live->physical_height = physical_height;
  live->scale_factor = scale_factor;
  ++live->generation;
  return MLN_STATUS_OK;
}

auto unsupported_retarget(const char* message) -> mln_status {
  set_thread_error(message);
  return MLN_STATUS_UNSUPPORTED;
}

auto validate_render_session_retarget(
  mln_render_session session, RetargetTargetKind kind,
  mln_render_session_object*& out_session
) -> mln_status {
  const auto status =
    validate_live_attached_render_session(session, out_session);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  if (kind == RetargetTargetKind::Surface) {
    if (out_session->kind != RenderSessionKind::Surface) {
      return unsupported_retarget(
        "session does not render through a native surface"
      );
    }
    return MLN_STATUS_OK;
  }

  if (out_session->kind != RenderSessionKind::Texture) {
    return unsupported_retarget(
      "session does not render into a caller-owned texture"
    );
  }
  if (out_session->texture.acquired) {
    set_thread_error(
      "cannot replace the render target while a texture frame is acquired"
    );
    return MLN_STATUS_INVALID_STATE;
  }
  if (out_session->texture.mode != TextureSessionMode::Borrowed) {
    return unsupported_retarget(
      "a session-owned texture is sized and replaced by its session; resize it "
      "instead"
    );
  }
  return MLN_STATUS_OK;
}

auto render_session_set_target(
  mln_render_session session, RetargetTargetKind kind,
  const mln_render_target_extent& extent, uint32_t physical_width,
  uint32_t physical_height, const RenderTargetReplacer& replace
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_render_session_retarget(session, kind, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  const auto replace_status = [&]() -> mln_status {
    try {
      return replace(*live);
    } catch (...) {
      // A throw means the swap was already under way and cannot be unwound, so
      // whatever the renderer caches against may be gone. Retire the renderer
      // before the exception reaches the C boundary; the host destroys the
      // session.
      live->renderer.reset();
      throw;
    }
  }();
  // Backends validate before they touch anything, so a reported failure leaves
  // the target and the renderer as they were.
  if (replace_status != MLN_STATUS_OK) {
    return replace_status;
  }

  // Land the session's own bookkeeping before anything that can fail again, so
  // a later failure leaves only a session waiting for the map to catch up.
  //
  // As with a resize, the renderer carries its caches to the new target. Pixel
  // ratio is the exception: it is baked into the renderer's shaders.
  if (extent.scale_factor != live->scale_factor) {
    live->renderer.reset();
  }
  live->rendered_generation = 0;
  live->width = extent.width;
  live->height = extent.height;
  live->physical_width = physical_width;
  live->physical_height = physical_height;
  live->scale_factor = extent.scale_factor;
  if (live->kind == RenderSessionKind::Texture) {
    live->texture.rendered_native_texture = nullptr;
    live->texture.acquired_native_texture = nullptr;
    live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  }
  ++live->generation;

  const auto size_status =
    map_post_set_size(live->map, extent.width, extent.height);
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  warn_on_scale_factor_mismatch(live->map, extent.scale_factor);
  return MLN_STATUS_OK;
}

auto surface_session_set_target(
  mln_render_session session, const mln_render_target_extent& extent,
  const RenderTargetReplacer& replace
) -> mln_status {
  const auto physical_status = validate_physical_size(
    extent.width, extent.height, extent.scale_factor,
    "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  return render_session_set_target(
    session, RetargetTargetKind::Surface, extent,
    physical_dimension(extent.width, extent.scale_factor),
    physical_dimension(extent.height, extent.scale_factor), replace
  );
}

auto render_session_render_update(
  mln_render_session session, mln_render_result* out_result,
  bool* out_needs_repaint
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_result == nullptr) {
    set_thread_error("out_result must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_needs_repaint == nullptr) {
    set_thread_error("out_needs_repaint must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_result = MLN_RENDER_RESULT_NO_UPDATE;
  *out_needs_repaint = false;
  if (live->kind == RenderSessionKind::Texture && live->texture.acquired) {
    set_thread_error("cannot render while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }

  auto* backend = renderer_backend(live);
  if (backend == nullptr) {
    set_thread_error("render session renderer backend is not available");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  // Deliver tile and resource results first: destroying the tiles they retire
  // enqueues the GPU-release work that map_run_render_jobs() drains. This must
  // stay ahead of the early returns below, or a frame with no update to render
  // strands them.
  live->scheduler.drain();
  map_run_render_jobs(live->map);

  auto update = map_latest_update(live->map);
  if (!update) {
    *out_result = MLN_RENDER_RESULT_NO_UPDATE;
    return MLN_STATUS_OK;
  }

  // The map applies its logical size on its own thread, so after a resize an
  // update built for the previous extent would take its projection from the
  // update but its viewport from the backend, producing a stretched frame.
  // Waiting cannot stall: Transform::resize publishes a new update unless the
  // size already matches.
  if (
    update->transformState.getSize() != mbgl::Size{live->width, live->height}
  ) {
    *out_result = MLN_RENDER_RESULT_SIZE_PENDING;
    return MLN_STATUS_OK;
  }

  if (live->kind == RenderSessionKind::Texture) {
    live->texture.backend->prepare_render_resources();
  } else {
    bool surface_ready = true;
    try {
      const auto prepare_status =
        live->surface.backend->prepare_frame(surface_ready);
      if (prepare_status != MLN_STATUS_OK) {
        return prepare_status;
      }
    } catch (const std::exception& exception) {
      set_native_stage_error("preparing surface frame", exception);
      return MLN_STATUS_NATIVE_ERROR;
    }
    if (!surface_ready) {
      *out_result = MLN_RENDER_RESULT_TARGET_NOT_READY;
      return MLN_STATUS_OK;
    }
  }
  if (live->renderer == nullptr) {
    try {
      live->renderer = std::make_unique<mbgl::Renderer>(
        *backend, static_cast<float>(live->scale_factor)
      );
      live->frame_observer.set_delegate(map_renderer_observer(live->map));
      live->renderer->setObserver(&live->frame_observer);
    } catch (const std::exception& exception) {
      set_native_stage_error("creating renderer", exception);
      return MLN_STATUS_NATIVE_ERROR;
    }
  }

  try {
    live->renderer->render(update);
  } catch (const std::exception& exception) {
    set_native_stage_error("rendering update", exception);
    return MLN_STATUS_NATIVE_ERROR;
  }
  // Absorb results that landed from worker threads during the render.
  live->scheduler.drain();
  if (live->kind == RenderSessionKind::Texture) {
    auto frame_rendered = true;
    const auto after_status =
      live->texture.backend->after_render(*live, frame_rendered);
    if (after_status != MLN_STATUS_OK) {
      return after_status;
    }
    if (!frame_rendered) {
      // A Metal owned-texture session can finish a pass before its texture
      // exists while the map is still loading. The target is valid, so this is
      // map-update driven rather than target-not-ready.
      *out_result = MLN_RENDER_RESULT_NO_UPDATE;
      return MLN_STATUS_OK;
    }
  }
  live->rendered_generation = live->generation;
  *out_result = MLN_RENDER_RESULT_RENDERED;
  *out_needs_repaint = live->frame_observer.needs_repaint();
  return MLN_STATUS_OK;
}

auto render_session_detach(mln_render_session session) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (live->kind == RenderSessionKind::Texture && live->texture.acquired) {
    set_thread_error("cannot detach while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }

  live->scheduler.set_repaint_request({});

  // Tear the renderer down before releasing the map's slot. The renderer holds
  // the map's forwarding observer, which the map's frontend owns; releasing the
  // slot first lets the map owner thread destroy the map and free that observer
  // underneath the drain and reset below.
  {
    auto current = ScopedCurrentScheduler{live->scheduler};
    live->scheduler.drain();
    live->renderer.reset();
    live->surface.backend.reset();
    live->texture.backend.reset();
    // Anything enqueued during teardown targets something already gone.
    live->scheduler.discard();
  }

  const auto detach_status = map_detach_render_target_session(live->map, live);
  if (detach_status != MLN_STATUS_OK) {
    return detach_status;
  }
  live->attached = false;
  live->rendered_generation = 0;
  live->texture.rendered_native_texture = nullptr;
  live->texture.acquired_native_texture = nullptr;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  ++live->generation;
  return MLN_STATUS_OK;
}

auto render_session_destroy(mln_render_session session) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (live->kind == RenderSessionKind::Texture && live->texture.acquired) {
    set_thread_error("cannot destroy while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->attached) {
    const auto detach_status = render_session_detach(session);
    if (detach_status != MLN_STATUS_OK) {
      return detach_status;
    }
  }
  auto owned_session = erase_render_session(session);
  owned_session.reset();
  return MLN_STATUS_OK;
}

auto render_session_reduce_memory_use(mln_render_session session)
  -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  live->renderer->reduceMemoryUse();
  return MLN_STATUS_OK;
}

auto render_session_clear_data(mln_render_session session) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  live->renderer->clearData();
  return MLN_STATUS_OK;
}

auto render_session_dump_debug_logs(mln_render_session session) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  live->renderer->dumpDebugLogs();
  return MLN_STATUS_OK;
}

auto render_session_set_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector,
  mln_buffer_view state
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
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

  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  live->renderer->setFeatureState(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    string_from_view(selector->feature_id), *state_object
  );
  static_cast<void>(map_post_trigger_repaint(live->map));
  return MLN_STATUS_OK;
}

auto render_session_get_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector,
  mln_buffer* out_state
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto state = mbgl::FeatureState{};
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  live->renderer->getFeatureState(
    state, string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    string_from_view(selector->feature_id)
  );
  return create_buffer(
    serialize_json_value(mbgl::Value{std::move(state)}), out_state
  );
}

auto render_session_remove_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, false);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  mbgl::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  live->renderer->removeFeatureState(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, selector->feature_id
    ),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY, selector->state_key
    )
  );
  static_cast<void>(map_post_trigger_repaint(live->map));
  return MLN_STATUS_OK;
}

auto queried_feature_list_count(
  mln_queried_feature_list list, size_t* out_count
) -> mln_status {
  if (out_count == nullptr) {
    set_thread_error("out_count must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& table = handle_table<QueriedFeatureListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_count = live_list->features.size();
  return MLN_STATUS_OK;
}

auto queried_feature_list_get(
  mln_queried_feature_list list, size_t index, mln_queried_feature* out_feature
) -> mln_status {
  if (
    out_feature == nullptr || out_feature->size < sizeof(mln_queried_feature)
  ) {
    set_thread_error("out_feature must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& table = handle_table<QueriedFeatureListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (index >= live_list->features.size()) {
    set_thread_error("index is out of range");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto& record = live_list->features.at(index);
  const auto view = [](const std::string& string) -> mln_buffer_view {
    return {.data = string.data(), .size = string.size()};
  };
  *out_feature = mln_queried_feature{};
  out_feature->size = sizeof(mln_queried_feature);
  out_feature->fields = record.fields;
  out_feature->feature = view(record.feature);
  if ((record.fields & MLN_QUERIED_FEATURE_SOURCE_ID) != 0) {
    out_feature->source_id = view(record.source_id);
  }
  if ((record.fields & MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0) {
    out_feature->source_layer_id = view(record.source_layer_id);
  }
  if ((record.fields & MLN_QUERIED_FEATURE_STATE) != 0) {
    out_feature->state = view(record.state);
  }
  return MLN_STATUS_OK;
}

auto queried_feature_list_destroy(mln_queried_feature_list list) -> void {
  static_cast<void>(handle_table<QueriedFeatureListObject>().remove(list));
}

auto render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_queried_feature_list* out_result
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto output_status = validate_result_output(out_result);
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
        geometry->data.box, live->width, live->height
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
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  auto features = std::vector<mbgl::Feature>{};
  switch (geometry->type) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      features = live->renderer->queryRenderedFeatures(
        mbgl::ScreenCoordinate{geometry->data.point.x, geometry->data.point.y},
        *native_options
      );
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX:
      if (clipped_box) {
        features =
          live->renderer->queryRenderedFeatures(*clipped_box, *native_options);
      }
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING:
      features =
        live->renderer->queryRenderedFeatures(line_string, *native_options);
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
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  mln_queried_feature_list* out_result
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto output_status = validate_result_output(out_result);
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
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto native_source_id = string_from_view(source_id);
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  auto features =
    live->renderer->querySourceFeatures(native_source_id, *native_options);
  return create_feature_query_result(
    std::move(features), native_source_id, out_result
  );
}

auto render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_buffer* out_result
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto output_status = validate_result_output(out_result);
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
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto query_feature = mbgl::Feature{std::move(*native_feature)};
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*backend};
  auto result = live->renderer->queryFeatureExtensions(
    string_from_view(source_id), query_feature, string_from_view(extension),
    string_from_view(extension_field), std::move(*native_arguments)
  );
  return create_feature_extension_result(std::move(result), out_result);
}

}  // namespace mln::core
