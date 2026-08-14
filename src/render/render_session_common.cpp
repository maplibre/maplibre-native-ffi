#include <algorithm>
#include <chrono>
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
#include <mbgl/style/conversion/stringify.hpp>
#include <mbgl/style/filter.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/geojson.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/util/string.hpp>

#include <mapbox/geojson/rapidjson.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "render/render_session_common.hpp"

#include "bytes/buffer.hpp"
#include "c_api/autorelease_pool.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "notification/notification.hpp"
#include "operation/operation.hpp"
#include "render/render_session_test_support.hpp"
#include "runtime/runtime.hpp"
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
    .kind = MLN_WEBGL_CONTEXT_EXISTING,
    .context = 0,
    .canvas_selector = {},
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
    if (context.data.webgl.kind == MLN_WEBGL_CONTEXT_EXISTING) {
      if (dedicated) {
        set_thread_error("an existing WebGL context must use shared ownership");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (context.data.webgl.context <= 0) {
        set_thread_error("WebGL context handle must be positive");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      return MLN_STATUS_OK;
    }
    if (context.data.webgl.kind == MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS) {
      if (!dedicated) {
        set_thread_error(
          "a transferred WebGL canvas must use dedicated ownership"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (
        context.data.webgl.context != 0 ||
        context.data.webgl.canvas_selector.data == nullptr ||
        context.data.webgl.canvas_selector.size == 0
      ) {
        set_thread_error(
          "a transferred WebGL canvas requires a selector and no context handle"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      return MLN_STATUS_OK;
    }
    set_thread_error("WebGL context kind is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
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
      if (lhs.data.webgl.kind != rhs.data.webgl.kind) {
        return false;
      }
      if (lhs.data.webgl.kind == MLN_WEBGL_CONTEXT_EXISTING) {
        return lhs.data.webgl.context == rhs.data.webgl.context;
      }
      return lhs.data.webgl.canvas_selector.size ==
               rhs.data.webgl.canvas_selector.size &&
             std::equal(
               static_cast<const std::byte*>(
                 lhs.data.webgl.canvas_selector.data
               ),
               static_cast<const std::byte*>(
                 lhs.data.webgl.canvas_selector.data
               ) +
                 lhs.data.webgl.canvas_selector.size,
               static_cast<const std::byte*>(
                 rhs.data.webgl.canvas_selector.data
               )
             );
    case MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED:
      break;
  }
  return false;
}

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

auto create_feature_query_result(
  std::vector<mbgl::Feature> features,
  const std::optional<std::string>& source_id, mln_buffer* out_result
) -> mln_status {
  const auto output_status = validate_result_output(out_result);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  auto buffer = rapidjson::StringBuffer{};
  auto writer = rapidjson::Writer<rapidjson::StringBuffer>{buffer};
  writer.StartArray();
  for (const auto& feature : features) {
    writer.StartObject();
    writer.Key("feature");
    auto allocator = mapbox::geojson::rapidjson_allocator{};
    auto feature_json = mapbox::geojson::convert(
      static_cast<const mbgl::GeoJSONFeature&>(feature), allocator
    );
    feature_json.Accept(writer);
    const auto effective_source_id =
      feature.source.empty() && source_id ? *source_id : feature.source;
    if (!effective_source_id.empty()) {
      writer.Key("sourceId");
      writer.String(
        effective_source_id.data(),
        static_cast<rapidjson::SizeType>(effective_source_id.size())
      );
    }
    if (!feature.sourceLayer.empty()) {
      writer.Key("sourceLayerId");
      writer.String(
        feature.sourceLayer.data(),
        static_cast<rapidjson::SizeType>(feature.sourceLayer.size())
      );
    }
    if (!feature.state.empty()) {
      writer.Key("state");
      mbgl::style::conversion::stringify(writer, mbgl::Value{feature.state});
    }
    writer.EndObject();
  }
  writer.EndArray();
  return mln::core::create_buffer(
    std::string{buffer.GetString(), buffer.GetSize()}, out_result
  );
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

namespace {

auto finish_driver_work(
  const std::shared_ptr<OperationObject>& operation,
  const RenderDriverCallable& callable,
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void {
  try {
    const auto status = callable(*session);
    operation->complete(status, {}, {});
  } catch (const std::exception& exception) {
    operation->complete(MLN_STATUS_NATIVE_ERROR, exception.what(), {});
  } catch (...) {
    operation->complete(
      MLN_STATUS_NATIVE_ERROR, "render driver work failed", {}
    );
  }
}

auto publish_driver_work_locked(mln_render_session_object& session) noexcept
  -> void {
  if (session.capabilities.driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    session.worker_condition.notify_one();
  } else if (session.driver_endpoint != nullptr) {
    session.driver_endpoint->mark_ready();
  }
}
auto driver_work_kind(std::uint32_t operation_kind) noexcept
  -> RenderDriverWorkKind {
  switch (operation_kind) {
    case RENDER_OPERATION_RESIZE:
      return RenderDriverWorkKind::Resize;
    case RENDER_OPERATION_BARRIER:
      return RenderDriverWorkKind::Barrier;
    case RENDER_OPERATION_QUERY:
    case RENDER_OPERATION_READBACK:
      return RenderDriverWorkKind::Query;
    case RENDER_OPERATION_FEATURE_STATE_GET:
    case RENDER_OPERATION_FEATURE_STATE_SET:
    case RENDER_OPERATION_FEATURE_STATE_REMOVE:
      return RenderDriverWorkKind::FeatureState;
    case RENDER_OPERATION_FRAME_RELEASE:
      return RenderDriverWorkKind::FrameRelease;
    case RENDER_OPERATION_DETACH:
      return RenderDriverWorkKind::Detach;
    default:
      return RenderDriverWorkKind::Maintenance;
  }
}
auto make_driver_work(
  RenderDriverWorkKind kind, std::function<void()> execute,
  std::function<void()> abandon
) -> RenderDriverWorkItem {
  const auto callbacks =
    DriverWorkCallbacks{std::move(execute), std::move(abandon)};
  switch (kind) {
    case RenderDriverWorkKind::Attach:
      return AttachDriverWork{callbacks};
    case RenderDriverWorkKind::FrameDemand:
      return FrameDemandDriverWork{callbacks};
    case RenderDriverWorkKind::Resize:
      return ResizeDriverWork{callbacks};
    case RenderDriverWorkKind::Barrier:
      return BarrierDriverWork{callbacks};
    case RenderDriverWorkKind::Query:
      return QueryDriverWork{callbacks};
    case RenderDriverWorkKind::FeatureState:
      return FeatureStateDriverWork{callbacks};
    case RenderDriverWorkKind::Retarget:
      return RetargetDriverWork{{}, callbacks};
    case RenderDriverWorkKind::FrameRelease:
      return FrameReleaseDriverWork{callbacks};
    case RenderDriverWorkKind::Detach:
      return DetachDriverWork{callbacks};
    case RenderDriverWorkKind::Maintenance:
      return MaintenanceDriverWork{callbacks};
  }
  return MaintenanceDriverWork{callbacks};
}

auto driver_work_callbacks(RenderDriverWorkItem& item) noexcept
  -> DriverWorkCallbacks& {
  return std::visit(
    [](auto& payload) -> DriverWorkCallbacks& { return payload.callbacks; },
    item
  );
}

auto run_core_worker(
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void {
  while (true) {
    auto work = RenderDriverWorkItem{};
    {
      auto lock = std::unique_lock{session->control_mutex};
      session->worker_condition.wait(lock, [&]() noexcept {
        return session->stop_worker || !session->driver_work.empty();
      });
      if (session->stop_worker && session->driver_work.empty()) return;
      work = std::move(session->driver_work.front());
      session->driver_work.pop_front();
      session->driver_call_in_flight = true;
    }
    auto& callbacks = driver_work_callbacks(work);
    try {
      auto execute = [&]() -> mln_status {
        if (callbacks.execute) callbacks.execute();
        return MLN_STATUS_OK;
      };
      static_cast<void>(mln::c_api::with_autorelease_pool(execute));
    } catch (...) {
      if (callbacks.abandon) callbacks.abandon();
    }
    {
      const auto lock = std::scoped_lock{session->control_mutex};
      session->driver_call_in_flight = false;
    }
  }
}

auto enqueue_work(
  const std::shared_ptr<mln_render_session_object>& session,
  RenderDriverWorkItem work
) -> void {
  const auto lock = std::scoped_lock{session->control_mutex};
  auto& queue = session->waiting_update_work.empty()
                  ? session->driver_work
                  : session->waiting_update_work;
  queue.push_back(std::move(work));
  if (session->waiting_update_work.empty())
    publish_driver_work_locked(*session);
}

}  // namespace

auto lease_render_session(mln_render_session session)
  -> std::shared_ptr<mln_render_session_object> {
  return handle_table<mln_render_session_object>().lease(session);
}

auto enqueue_driver_operation(
  mln_render_session session, std::uint32_t operation_kind,
  RenderDriverCallable work, mln_operation* out_operation
) -> mln_status {
  if (out_operation == nullptr || *out_operation != MLN_HANDLE_NULL) {
    set_thread_error("out_operation must point to a null operation handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = lease_render_session(session);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto operation = std::shared_ptr<OperationObject>{};
  const auto status = register_operation(
    live->operation_source, operation_kind, false, {}, out_operation, operation
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  enqueue_work(
    live, make_driver_work(
            driver_work_kind(operation_kind),
            [live, operation, work = std::move(work)]() {
              finish_driver_work(operation, work, live);
            },
            [operation]() {
              operation->complete(
                MLN_STATUS_TARGET_LOST, "render target was abandoned", {}
              );
            }
          )
  );
  return MLN_STATUS_OK;
}
auto enqueue_driver_result_operation(
  mln_render_session session, std::uint32_t operation_kind,
  RenderDriverResultCallable work, mln_operation* out_operation
) -> mln_status {
  if (out_operation == nullptr || *out_operation != MLN_HANDLE_NULL) {
    set_thread_error("out_operation must point to a null operation handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = lease_render_session(session);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto operation = std::shared_ptr<OperationObject>{};
  const auto status = register_operation(
    live->operation_source, operation_kind, false, {}, out_operation, operation
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  enqueue_work(
    live, make_driver_work(
            driver_work_kind(operation_kind),
            [live, operation, work = std::move(work)]() {
              try {
                auto result = std::any{};
                const auto work_status = work(*live, result);
                operation->complete(
                  work_status,
                  work_status == MLN_STATUS_OK
                    ? std::string{}
                    : std::string{thread_last_error_message()},
                  std::move(result)
                );
              } catch (const std::exception& exception) {
                operation->complete(
                  MLN_STATUS_NATIVE_ERROR, exception.what(), std::any{}
                );
              }
            },
            [operation]() {
              operation->complete(
                MLN_STATUS_TARGET_LOST, "render target was abandoned",
                std::any{}
              );
            }
          )
  );
  return MLN_STATUS_OK;
}
auto enqueue_blocking_test_render_operation(
  mln_render_session session, std::atomic_bool* entered,
  const std::atomic_bool* release, mln_operation* out_operation
) -> mln_status {
  if (entered == nullptr || release == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return enqueue_driver_operation(
    session, RENDER_OPERATION_MAINTENANCE,
    [entered, release](mln_render_session_object&) {
      entered->store(true, std::memory_order_release);
      while (!release->load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      return MLN_STATUS_OK;
    },
    out_operation
  );
}

auto validate_render_session_attach_request(
  const mln_render_session_attach_options* options,
  const mln_render_session* out_session, const mln_operation* out_operation
) -> mln_status {
  if (options == nullptr) {
    set_thread_error("render session attach options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_render_session_attach_options)) {
    set_thread_error("render session attach options size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    out_session == nullptr || *out_session != MLN_HANDLE_NULL ||
    out_operation == nullptr || *out_operation != MLN_HANDLE_NULL
  ) {
    set_thread_error("attach outputs must point to null handles");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    options->driver != MLN_RENDER_DRIVER_CORE_WORKER &&
    options->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ) {
    set_thread_error("render driver kind is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto start_attach_render_session(
  std::shared_ptr<mln_render_session_object> session, RenderSessionKind kind,
  const mln_render_session_attach_options* options,
  mln_render_session_capabilities capabilities, mln_render_session* out_session,
  mln_operation* out_operation
) -> mln_status {
  if (session == nullptr) {
    set_thread_error("render session must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto request_status =
    validate_render_session_attach_request(options, out_session, out_operation);
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }
  if (capabilities.size < sizeof(mln_render_session_capabilities)) {
    set_thread_error("render session capabilities size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto map = handle_table<MapObject>().lease(session->map);
  if (
    map == nullptr || map->runtime_state == nullptr ||
    map->runtime_state->event_queue == nullptr
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto operation_source =
    options->operation_source == MLN_HANDLE_NULL
      ? map->runtime_state->event_queue->notification_source
      : notification_source_from_handle(options->operation_source);
  if (operation_source == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto frame_source =
    options->frame_source == MLN_HANDLE_NULL
      ? operation_source
      : notification_source_from_handle(options->frame_source);
  auto driver_source =
    options->driver_work_source == MLN_HANDLE_NULL
      ? operation_source
      : notification_source_from_handle(options->driver_work_source);
  if (frame_source == nullptr || driver_source == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto attach_operation = std::shared_ptr<OperationObject>{};
  const auto operation_status = register_operation(
    operation_source, RENDER_OPERATION_ATTACH, false, {}, out_operation,
    attach_operation
  );
  if (operation_status != MLN_STATUS_OK) {
    return operation_status;
  }
  session->kind = kind;
  session->capabilities = capabilities;
  if (
    kind == RenderSessionKind::Texture &&
    session->texture.mode == TextureSessionMode::Owned
  ) {
    const auto depth = std::clamp(capabilities.texture_ring_depth, 1u, 3u);
    session->capabilities.texture_ring_depth = depth;
    session->texture.slots.resize(depth);
  }
  session->capabilities.driver = options->driver;
  session->operation_source = std::move(operation_source);
  session->state = MLN_RENDER_SESSION_STATE_ATTACHING;
  const auto attach_status =
    map_attach_render_target_session(session->map, session.get());
  if (attach_status != MLN_STATUS_OK) {
    abandon_operation(*out_operation);
    *out_operation = MLN_HANDLE_NULL;
    return attach_status;
  }
  session->attached = true;
  session->self = register_render_session(session);
  session->frame_endpoint = frame_source->associate(
    session->self, MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES, true
  );
  session->driver_endpoint = driver_source->associate(
    session->self, MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK, true
  );
  if (
    session->frame_endpoint == nullptr || session->driver_endpoint == nullptr
  ) {
    session->frame_endpoint.reset();
    session->driver_endpoint.reset();
    static_cast<void>(
      map_detach_render_target_session(session->map, session.get())
    );
    static_cast<void>(
      handle_table<mln_render_session_object>().remove(session->self)
    );
    abandon_operation(*out_operation);
    *out_operation = MLN_HANDLE_NULL;
    return MLN_STATUS_INVALID_STATE;
  }
  const auto weak_session = std::weak_ptr<mln_render_session_object>{session};
  const auto publish_status = map_set_render_session_publish_callback(
    session->map, [weak_session]() noexcept {
      if (const auto live = weak_session.lock()) {
        notify_render_session_map_update(live.get());
      }
    }
  );
  if (publish_status != MLN_STATUS_OK) {
    session->frame_endpoint.reset();
    session->driver_endpoint.reset();
    static_cast<void>(
      map_detach_render_target_session(session->map, session.get())
    );
    static_cast<void>(
      handle_table<mln_render_session_object>().remove(session->self)
    );
    abandon_operation(*out_operation);
    *out_operation = MLN_HANDLE_NULL;
    return publish_status;
  }
  try {
    if (options->driver == MLN_RENDER_DRIVER_CORE_WORKER) {
      if (session->start_worker) {
        const auto worker_status =
          session->start_worker([session]() { run_core_worker(session); });
        if (worker_status != MLN_STATUS_OK) {
          session->frame_endpoint.reset();
          session->driver_endpoint.reset();
          static_cast<void>(
            map_set_render_session_publish_callback(session->map, {})
          );
          static_cast<void>(
            map_detach_render_target_session(session->map, session.get())
          );
          static_cast<void>(
            handle_table<mln_render_session_object>().remove(session->self)
          );
          abandon_operation(*out_operation);
          *out_operation = MLN_HANDLE_NULL;
          return worker_status;
        }
      } else {
        session->worker =
          std::thread{[session]() { run_core_worker(session); }};
      }
    }
    enqueue_work(
      session,
      make_driver_work(
        RenderDriverWorkKind::Attach,
        [session, attach_operation]() {
          try {
            if (session->initialize_backend) {
              const auto initialize_status =
                session->initialize_backend(*session);
              if (initialize_status != MLN_STATUS_OK) {
                {
                  const auto lock = std::scoped_lock{session->control_mutex};
                  session->state = MLN_RENDER_SESSION_STATE_TARGET_LOST;
                  session->target_ready = false;
                  ++session->generation;
                }
                attach_operation->complete(
                  initialize_status, thread_last_error_message(), {}
                );
                return;
              }
            }
            if (
              auto* backend = renderer_backend(session.get());
              backend != nullptr
            ) {
              const auto prime = mbgl::gfx::BackendScope{*backend};
            }
            {
              const auto lock = std::scoped_lock{session->control_mutex};
              session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
              ++session->generation;
            }
            static_cast<void>(map_post_trigger_repaint(session->map));
            attach_operation->complete(MLN_STATUS_OK, {}, {});
          } catch (const std::exception& exception) {
            {
              const auto lock = std::scoped_lock{session->control_mutex};
              session->state = MLN_RENDER_SESSION_STATE_TARGET_LOST;
              session->target_ready = false;
              ++session->generation;
            }
            attach_operation->complete(
              MLN_STATUS_NATIVE_ERROR, exception.what(), {}
            );
          }
        },
        [attach_operation]() {
          attach_operation->complete(
            MLN_STATUS_TARGET_LOST, "render target was abandoned", {}
          );
        }
      )
    );
  } catch (...) {
    {
      const auto lock = std::scoped_lock{session->control_mutex};
      session->stop_worker = true;
      session->worker_condition.notify_all();
    }
    if (session->join_worker)
      session->join_worker();
    else if (session->worker.joinable())
      session->worker.join();
    static_cast<void>(
      map_set_render_session_publish_callback(session->map, {})
    );
    session->frame_endpoint.reset();
    session->driver_endpoint.reset();
    static_cast<void>(
      map_detach_render_target_session(session->map, session.get())
    );
    static_cast<void>(
      handle_table<mln_render_session_object>().remove(session->self)
    );
    abandon_operation(*out_operation);
    *out_operation = MLN_HANDLE_NULL;
    throw;
  }
  *out_session = session->self;
  return MLN_STATUS_OK;
}

auto notify_render_session_map_update(
  mln_render_session_object* session
) noexcept -> void {
  if (session == nullptr) {
    return;
  }
  const auto lock = std::scoped_lock{session->control_mutex};
  session->map_update_generation = map_latest_update_generation(session->map);
  session->pending_changes = true;
  if (!session->waiting_update_work.empty()) {
    while (!session->waiting_update_work.empty()) {
      session->driver_work.push_back(
        std::move(session->waiting_update_work.front())
      );
      session->waiting_update_work.pop_front();
    }
    publish_driver_work_locked(*session);
  }
  if (
    !session->demands.empty() &&
    session->capabilities.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD &&
    session->driver_endpoint != nullptr
  ) {
    session->driver_endpoint->mark_ready();
  }
  session->worker_condition.notify_one();
}

auto validate_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status {
  out_session = handle_table<mln_render_session_object>().resolve(session);
  return out_session == nullptr ? MLN_STATUS_INVALID_ARGUMENT : MLN_STATUS_OK;
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
  const auto size_status = map_post_resize(
    live->map, mln_logical_extent{
                 .width = width, .height = height, .scale_factor = scale_factor
               }
  );
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
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
  live->rendered_target_generation = 0;
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

  // Target replacement changes only the graphics resource. Map creation and
  // explicit resize commands remain the sole extent authorities.
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

auto render_session_render_update_on_driver(
  mln_render_session session, mln_render_result* out_result
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
  *out_result = MLN_RENDER_RESULT_NO_UPDATE;
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
      live->renderer->setObserver(map_renderer_observer(live->map));
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
  live->rendered_target_generation = live->generation;
  *out_result = MLN_RENDER_RESULT_RENDERED;
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
  // slot first lets the runtime worker destroy the map and free that observer
  // while the drain and reset below still use it.
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
  live->rendered_target_generation = 0;
  live->texture.rendered_native_texture = nullptr;
  live->texture.acquired_native_texture = nullptr;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  ++live->generation;
  return MLN_STATUS_OK;
}

auto render_session_destroy(mln_render_session session) -> mln_status {
  const auto live = lease_render_session(session);
  if (!live) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (
      live->state != MLN_RENDER_SESSION_STATE_DETACHED &&
      live->state != MLN_RENDER_SESSION_STATE_ABANDONED
    )
      return MLN_STATUS_INVALID_STATE;
    if (
      live->state == MLN_RENDER_SESSION_STATE_DETACHED &&
      live->acquired_frame_count != 0
    )
      return MLN_STATUS_INVALID_STATE;
    live->stop_worker = true;
    live->worker_condition.notify_all();
  }
  if (live->join_worker)
    live->join_worker();
  else if (live->worker.joinable())
    live->worker.join();
  live->frame_endpoint.reset();
  live->driver_endpoint.reset();
  static_cast<void>(handle_table<mln_render_session_object>().remove(session));
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

auto render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options, mln_buffer* out_result
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
  const mln_source_feature_query_options* options, mln_buffer* out_result
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

namespace {
auto publish_frame_result(
  const std::shared_ptr<mln_render_session_object>& session,
  mln_render_frame_result result
) noexcept -> void {
  const auto lock = std::scoped_lock{session->control_mutex};
  session->latest_result = static_cast<mln_render_result>(result.disposition);
  session->latest_demand_token = result.token;
  session->frame_results.push_back(result);
  if (session->frame_endpoint) session->frame_endpoint->mark_ready();
}

auto run_frame_demand(
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void {
  auto pending = PendingFrameDemand{};
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    if (
      session->demands.empty() ||
      (session->state != MLN_RENDER_SESSION_STATE_ATTACHED &&
       session->state != MLN_RENDER_SESSION_STATE_DETACHING)
    )
      return;
    pending = session->demands.front();
    session->demands.pop_front();
    ++session->active_demand_count;
  }
  struct ActiveDemandGuard {
    std::shared_ptr<mln_render_session_object> session;
    ~ActiveDemandGuard() {
      const auto lock = std::scoped_lock{session->control_mutex};
      --session->active_demand_count;
    }
  } active_demand{session};
  const auto demand = pending.demand;
  auto result = mln_render_frame_result{
    .size = sizeof(mln_render_frame_result),
    .disposition = MLN_RENDER_RESULT_NO_UPDATE,
    .token = demand.token,
    .map_update_generation = map_latest_update_generation(session->map),
    .extent_generation = session->extent_generation,
    .frame_generation = 0,
    .presentation_time_ns = demand.presentation_time_ns,
  };
  const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()
  )
                        .count();
  if (demand.deadline_ns > 0 && now_ns >= demand.deadline_ns) {
    result.disposition = MLN_RENDER_RESULT_DEADLINE_MISSED;
    publish_frame_result(session, result);
    return;
  }
  auto selected_slot = std::optional<std::size_t>{};
  auto ring_full = false;
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    if (!session->texture.slots.empty()) {
      const auto reusable = std::find_if(
        session->texture.slots.begin(), session->texture.slots.end(),
        [](const RenderTextureSlot& value) {
          return !value.acquired && !value.available && !value.rendering;
        }
      );
      const auto slot =
        reusable != session->texture.slots.end()
          ? reusable
          : std::min_element(
              session->texture.slots.begin(), session->texture.slots.end(),
              [](
                const RenderTextureSlot& left, const RenderTextureSlot& right
              ) {
                if (left.acquired || left.rendering) return false;
                if (right.acquired || right.rendering) return true;
                return left.result.frame_generation <
                       right.result.frame_generation;
              }
            );
      ring_full = slot == session->texture.slots.end() || slot->acquired ||
                  slot->rendering;
      if (!ring_full) {
        selected_slot =
          static_cast<std::size_t>(slot - session->texture.slots.begin());
        slot->available = false;
        slot->rendering = true;
      }
    }
  }
  if (ring_full) {
    const auto lock = std::scoped_lock{session->control_mutex};
    session->demands.push_front(pending);
    return;
  }
  if (
    selected_slot && session->texture.backend &&
    session->texture.backend->select_render_slot(*selected_slot) !=
      MLN_STATUS_OK
  ) {
    {
      const auto lock = std::scoped_lock{session->control_mutex};
      session->texture.slots[*selected_slot].rendering = false;
    }
    result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
    publish_frame_result(session, result);
    return;
  }
  auto disposition = MLN_RENDER_RESULT_NO_UPDATE;
  if (
    render_session_render_update_on_driver(session->self, &disposition) !=
    MLN_STATUS_OK
  )
    disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
  result.disposition = disposition;
  if (disposition == MLN_RENDER_RESULT_RENDERED) {
    const auto lock = std::scoped_lock{session->control_mutex};
    result.frame_generation = ++session->frame_generation;
    session->rendered_generation = result.map_update_generation;
    session->pending_changes = false;
    if (selected_slot) {
      auto& slot = session->texture.slots[*selected_slot];
      slot.result = result;
      slot.available = true;
      slot.rendering = false;
    }
  }
  if (result.disposition != MLN_RENDER_RESULT_RENDERED && selected_slot) {
    const auto lock = std::scoped_lock{session->control_mutex};
    session->texture.slots[*selected_slot].rendering = false;
  }
  publish_frame_result(session, result);
}
}  // namespace

auto render_session_get_capabilities(
  mln_render_session handle, mln_render_session_capabilities* out
) -> mln_status {
  if (!out || out->size < sizeof(*out)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto session = lease_render_session(handle);
  if (!session) return MLN_STATUS_INVALID_ARGUMENT;
  const auto lock = std::scoped_lock{session->control_mutex};
  *out = session->capabilities;
  return MLN_STATUS_OK;
}

auto render_session_get_snapshot(
  mln_render_session handle, mln_render_session_snapshot* out
) -> mln_status {
  if (!out || out->size < sizeof(*out)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  const auto lock = std::scoped_lock{s->control_mutex};
  *out = mln_render_session_snapshot{
    .size = sizeof(*out),
    .state = s->state,
    .driver = s->capabilities.driver,
    .latest_result = s->latest_result,
    .extent = s->pending_extent.value_or(
      mln_render_target_extent{
        sizeof(mln_render_target_extent), s->width, s->height, s->scale_factor
      }
    ),
    .generation = s->generation,
    .map_update_generation = s->map_update_generation,
    .rendered_update_generation = s->rendered_generation,
    .extent_generation = s->extent_generation,
    .frame_generation = s->frame_generation,
    .latest_demand_token = s->latest_demand_token,
    .pending_demand_count =
      static_cast<uint32_t>(s->demands.size()) + s->active_demand_count,
    .acquired_frame_count = s->acquired_frame_count,
    .target_ready = s->target_ready,
    .pending_changes = s->pending_changes,
  };
  return MLN_STATUS_OK;
}

auto render_session_request_frame(
  mln_render_session handle, const mln_frame_demand* demand
) -> mln_status {
  if (!demand || demand->size < sizeof(*demand))
    return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  auto superseded = std::optional<mln_render_frame_result>{};
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->state != MLN_RENDER_SESSION_STATE_ATTACHED)
      return MLN_STATUS_INVALID_STATE;
    if (
      !s->demands.empty() &&
      s->demands.back().barrier_epoch == s->barrier_epoch &&
      s->demands.back().demand.flags == demand->flags &&
      s->demands.back().demand.coalescing_boundary ==
        demand->coalescing_boundary
    ) {
      const auto old = s->demands.back().demand;
      s->demands.pop_back();
      superseded = mln_render_frame_result{sizeof(mln_render_frame_result),
                                           MLN_RENDER_RESULT_SUPERSEDED,
                                           old.token,
                                           s->map_update_generation,
                                           s->extent_generation,
                                           0,
                                           old.presentation_time_ns};
    }
    s->demands.push_back(
      PendingFrameDemand{
        .demand = *demand,
        .barrier_epoch = s->barrier_epoch,
      }
    );
  }
  if (superseded) publish_frame_result(s, *superseded);
  enqueue_work(
    s, make_driver_work(
         RenderDriverWorkKind::FrameDemand, [s]() { run_frame_demand(s); }, {}
       )
  );
  return MLN_STATUS_OK;
}

auto render_session_service_driver_work(
  mln_render_session handle, std::size_t maximum, std::size_t* serviced
) -> mln_status {
  if (!serviced) return MLN_STATUS_INVALID_ARGUMENT;
  *serviced = 0;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->capabilities.driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD)
      return MLN_STATUS_INVALID_STATE;
    if (s->graphics_thread && *s->graphics_thread != std::this_thread::get_id())
      return MLN_STATUS_WRONG_THREAD;
    if (s->driver_call_in_flight) return MLN_STATUS_BUSY;
    if (!s->graphics_thread) s->graphics_thread = std::this_thread::get_id();
    s->driver_call_in_flight = true;
  }
  struct DriverCallGuard {
    std::shared_ptr<mln_render_session_object> session;
    ~DriverCallGuard() {
      const auto lock = std::scoped_lock{session->control_mutex};
      session->driver_call_in_flight = false;
      if (session->driver_endpoint) {
        if (session->driver_work.empty())
          session->driver_endpoint->clear_ready();
        else
          session->driver_endpoint->mark_ready();
      }
    }
  } guard{s};
  while (maximum == 0 || *serviced < maximum) {
    auto item = RenderDriverWorkItem{};
    {
      const auto lock = std::scoped_lock{s->control_mutex};
      if (s->driver_work.empty()) break;
      item = std::move(s->driver_work.front());
      s->driver_work.pop_front();
    }
    auto& callbacks = driver_work_callbacks(item);
    if (callbacks.execute) callbacks.execute();
    ++*serviced;
  }
  return MLN_STATUS_OK;
}

auto render_session_drain_frame_results(
  mln_render_session handle, std::size_t maximum, mln_render_frame_batch* out
) -> mln_status {
  if (!out || *out != MLN_HANDLE_NULL) return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  auto batch = std::make_shared<mln_render_frame_batch_object>();
  batch->session = s;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->frame_batch_live) return MLN_STATUS_BUSY;
    if (s->frame_results.empty()) return MLN_STATUS_NOT_READY;
    s->frame_batch_live = true;
    while ((maximum == 0 || batch->results.size() < maximum) &&
           !s->frame_results.empty()) {
      batch->results.push_back(s->frame_results.front());
      s->frame_results.pop_front();
    }
    if (s->frame_results.empty() && s->frame_endpoint)
      s->frame_endpoint->clear_ready();
  }
  try {
    *out = handle_table<mln_render_frame_batch_object>().insert(batch);
  } catch (...) {
    const auto lock = std::scoped_lock{s->control_mutex};
    for (auto item = batch->results.rbegin(); item != batch->results.rend();
         ++item)
      s->frame_results.push_front(*item);
    s->frame_batch_live = false;
    if (s->frame_endpoint) s->frame_endpoint->mark_ready();
    throw;
  }
  return MLN_STATUS_OK;
}

auto render_frame_batch_count(mln_render_frame_batch handle, std::size_t* out)
  -> mln_status {
  if (!out) return MLN_STATUS_INVALID_ARGUMENT;
  const auto batch =
    handle_table<mln_render_frame_batch_object>().lease(handle);
  if (!batch) return MLN_STATUS_INVALID_ARGUMENT;
  *out = batch->results.size();
  return MLN_STATUS_OK;
}

auto render_frame_batch_get(
  mln_render_frame_batch handle, std::size_t index, mln_render_frame_result* out
) -> mln_status {
  if (!out || out->size < sizeof(*out)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto batch =
    handle_table<mln_render_frame_batch_object>().lease(handle);
  if (!batch || index >= batch->results.size())
    return MLN_STATUS_INVALID_ARGUMENT;
  *out = batch->results[index];
  return MLN_STATUS_OK;
}

auto render_frame_batch_release(mln_render_frame_batch handle) noexcept
  -> void {
  const auto batch =
    handle_table<mln_render_frame_batch_object>().remove(handle);
  if (!batch || !batch->session) return;
  const auto lock = std::scoped_lock{batch->session->control_mutex};
  batch->session->frame_batch_live = false;
  if (!batch->session->frame_results.empty() && batch->session->frame_endpoint)
    batch->session->frame_endpoint->mark_ready();
}

auto render_session_acquire_frame(
  mln_render_session handle, mln_acquired_frame* out
) -> mln_status {
  if (!out || *out != MLN_HANDLE_NULL) return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  auto frame = std::make_shared<mln_acquired_frame_object>();
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->state != MLN_RENDER_SESSION_STATE_ATTACHED)
      return MLN_STATUS_INVALID_STATE;
    if (
      (s->capabilities.flags &
       MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) == 0
    ) {
      set_thread_error(
        "render session does not expose acquired texture frames"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
    const auto slot = std::min_element(
      s->texture.slots.begin(), s->texture.slots.end(),
      [](const RenderTextureSlot& left, const RenderTextureSlot& right) {
        if (!left.available || left.acquired) return false;
        if (!right.available || right.acquired) return true;
        return left.result.frame_generation < right.result.frame_generation;
      }
    );
    if (slot == s->texture.slots.end() || !slot->available || slot->acquired)
      return MLN_STATUS_NOT_READY;
    frame->session = s;
    frame->slot = static_cast<std::size_t>(slot - s->texture.slots.begin());
    frame->result = slot->result;
    frame->producer_sync = slot->producer_sync;
    if (s->texture.backend) {
      const auto metadata_status = s->texture.backend->copy_slot_metadata(
        *s, frame->slot, frame->backend_metadata
      );
      if (metadata_status != MLN_STATUS_OK) return metadata_status;
    }
    slot->available = false;
    slot->acquired = true;
    ++s->acquired_frame_count;
  }
  try {
    *out = handle_table<mln_acquired_frame_object>().insert(frame);
  } catch (...) {
    const auto lock = std::scoped_lock{s->control_mutex};
    auto& slot = s->texture.slots[frame->slot];
    slot.available = true;
    slot.acquired = false;
    --s->acquired_frame_count;
    throw;
  }
  return MLN_STATUS_OK;
}

auto acquired_frame_get_result(
  mln_acquired_frame handle, mln_render_frame_result* out
) -> mln_status {
  if (!out || out->size < sizeof(*out)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto frame = handle_table<mln_acquired_frame_object>().lease(handle);
  if (!frame || !frame->valid.load()) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{frame->session->control_mutex};
    if (frame->session->state == MLN_RENDER_SESSION_STATE_ABANDONED)
      return MLN_STATUS_TARGET_LOST;
  }
  *out = frame->result;
  return MLN_STATUS_OK;
}

auto acquired_frame_get_producer_sync(
  mln_acquired_frame handle, mln_gpu_sync* out
) -> mln_status {
  if (!out || out->size < sizeof(*out)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto frame = handle_table<mln_acquired_frame_object>().lease(handle);
  if (!frame || !frame->valid.load()) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{frame->session->control_mutex};
    if (frame->session->state == MLN_RENDER_SESSION_STATE_ABANDONED)
      return MLN_STATUS_TARGET_LOST;
  }
  *out = frame->producer_sync;
  return MLN_STATUS_OK;
}

auto acquired_frame_release_start(
  mln_acquired_frame* handle, const mln_gpu_sync* sync,
  mln_operation* out_operation
) -> mln_status {
  if (
    !handle || *handle == MLN_HANDLE_NULL || !out_operation ||
    *out_operation != MLN_HANDLE_NULL
  )
    return MLN_STATUS_INVALID_ARGUMENT;
  const auto copied =
    sync ? *sync
         : mln_gpu_sync{
             sizeof(mln_gpu_sync), MLN_GPU_SYNC_CPU_COMPLETE, nullptr, 0
           };
  if (copied.size < sizeof(mln_gpu_sync)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto frame = handle_table<mln_acquired_frame_object>().lease(*handle);
  if (!frame) return MLN_STATUS_INVALID_ARGUMENT;
  auto claimed = true;
  if (!frame->valid.compare_exchange_strong(claimed, false))
    return MLN_STATUS_INVALID_STATE;
  auto operation = std::shared_ptr<OperationObject>{};
  const auto status = register_operation(
    frame->session->operation_source, RENDER_OPERATION_FRAME_RELEASE, false, {},
    out_operation, operation
  );
  if (status != MLN_STATUS_OK) {
    frame->valid.store(true);
    return status;
  }
  const auto consumed =
    handle_table<mln_acquired_frame_object>().remove(*handle);
  if (!consumed) {
    operation->complete(
      MLN_STATUS_INVALID_STATE, "acquired frame was already released", {}
    );
    return MLN_STATUS_INVALID_STATE;
  }
  *handle = MLN_HANDLE_NULL;
  const auto release = [consumed, operation, copied]() {
    auto status = MLN_STATUS_OK;
    {
      const auto lock = std::scoped_lock{consumed->session->control_mutex};
      if (consumed->session->state == MLN_RENDER_SESSION_STATE_ABANDONED)
        status = MLN_STATUS_TARGET_LOST;
    }
    if (status == MLN_STATUS_OK && consumed->session->texture.backend)
      status =
        consumed->session->texture.backend->release_consumer_sync(copied);
    auto resume_demand = false;
    {
      const auto lock = std::scoped_lock{consumed->session->control_mutex};
      if (consumed->slot < consumed->session->texture.slots.size())
        consumed->session->texture.slots[consumed->slot].acquired = false;
      if (consumed->session->acquired_frame_count)
        --consumed->session->acquired_frame_count;
      resume_demand = !consumed->session->demands.empty();
    }
    operation->complete(status, {}, {});
    if (resume_demand) {
      enqueue_work(
        consumed->session,
        make_driver_work(
          RenderDriverWorkKind::FrameDemand,
          [session = consumed->session]() { run_frame_demand(session); }, {}
        )
      );
    }
  };
  auto abandoned = false;
  {
    const auto lock = std::scoped_lock{frame->session->control_mutex};
    abandoned = frame->session->state == MLN_RENDER_SESSION_STATE_ABANDONED;
  }
  if (abandoned)
    release();
  else
    enqueue_work(
      frame->session,
      make_driver_work(RenderDriverWorkKind::FrameRelease, release, release)
    );
  return MLN_STATUS_OK;
}

namespace {
auto make_ordered_resize_work(
  const std::shared_ptr<mln_render_session_object>& session,
  const std::shared_ptr<OperationObject>& operation,
  mln_render_target_extent extent
) -> RenderDriverWorkItem {
  return make_driver_work(
    RenderDriverWorkKind::Resize,
    [session, operation, extent]() {
      auto update = map_latest_update(session->map);
      if (
        !update || update->transformState.getSize() !=
                     mbgl::Size{extent.width, extent.height}
      ) {
        auto lock = std::unique_lock{session->control_mutex};
        update = map_latest_update(session->map);
        if (
          !update || update->transformState.getSize() !=
                       mbgl::Size{extent.width, extent.height}
        ) {
          session->waiting_update_work.push_back(
            make_ordered_resize_work(session, operation, extent)
          );
          while (!session->driver_work.empty()) {
            session->waiting_update_work.push_back(
              std::move(session->driver_work.front())
            );
            session->driver_work.pop_front();
          }
          return;
        }
      }
      const auto physical_width =
        physical_dimension(extent.width, extent.scale_factor);
      const auto physical_height =
        physical_dimension(extent.height, extent.scale_factor);
      if (session->kind == RenderSessionKind::Surface)
        session->surface.backend->resize(physical_width, physical_height);
      else
        session->texture.backend->resize({physical_width, physical_height});
      {
        const auto lock = std::scoped_lock{session->control_mutex};
        if (extent.scale_factor != session->scale_factor)
          session->renderer.reset();
        session->width = extent.width;
        session->height = extent.height;
        session->physical_width = physical_width;
        session->physical_height = physical_height;
        session->scale_factor = extent.scale_factor;
        session->pending_extent.reset();
        ++session->extent_generation;
        ++session->generation;
      }
      operation->complete(MLN_STATUS_OK, {}, {});
    },
    [operation]() {
      operation->complete(MLN_STATUS_TARGET_LOST, "target abandoned", {});
    }
  );
}

auto make_ordered_barrier_work(
  const std::shared_ptr<mln_render_session_object>& session, uint64_t minimum,
  const std::shared_ptr<OperationObject>& operation
) -> RenderDriverWorkItem {
  return make_driver_work(
    RenderDriverWorkKind::Barrier,
    [session, minimum, operation]() {
      if (map_latest_update_generation(session->map) < minimum) {
        auto lock = std::unique_lock{session->control_mutex};
        if (map_latest_update_generation(session->map) < minimum) {
          session->waiting_update_work.push_back(
            make_ordered_barrier_work(session, minimum, operation)
          );
          while (!session->driver_work.empty()) {
            session->waiting_update_work.push_back(
              std::move(session->driver_work.front())
            );
            session->driver_work.pop_front();
          }
          return;
        }
      }
      operation->complete(MLN_STATUS_OK, {}, {});
    },
    [operation]() {
      operation->complete(MLN_STATUS_TARGET_LOST, "target abandoned", {});
    }
  );
}
}  // namespace

auto render_session_resize_start(
  mln_render_session handle, const mln_render_target_extent* extent,
  mln_operation* out
) -> mln_status {
  if (!extent || !out || *out != MLN_HANDLE_NULL)
    return MLN_STATUS_INVALID_ARGUMENT;
  const auto valid = validate_render_target_extent(
    *extent, "render target dimensions and scale factor must be positive"
  );
  if (valid != MLN_STATUS_OK) return valid;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->state != MLN_RENDER_SESSION_STATE_ATTACHED)
      return MLN_STATUS_INVALID_STATE;
    if (
      s->kind == RenderSessionKind::Texture &&
      s->texture.mode == TextureSessionMode::Borrowed
    ) {
      set_thread_error(
        "a caller-owned texture is sized by its owner; hand over a replacement "
        "with the borrowed-texture set_target function for this backend"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
  }
  auto operation = std::shared_ptr<OperationObject>{};
  const auto registered = register_operation(
    s->operation_source, RENDER_OPERATION_RESIZE, false, {}, out, operation
  );
  if (registered != MLN_STATUS_OK) return registered;
  const auto copied = *extent;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    s->pending_extent = copied;
    s->pending_changes = true;
  }
  const auto post = map_post_resize(
    s->map, mln_logical_extent{
              .width = copied.width,
              .height = copied.height,
              .scale_factor = copied.scale_factor
            }
  );
  if (post != MLN_STATUS_OK) {
    operation->complete(post, {}, {});
    return MLN_STATUS_OK;
  }
  enqueue_work(s, make_ordered_resize_work(s, operation, copied));
  return MLN_STATUS_OK;
}

auto render_session_barrier_start(
  mln_render_session handle, uint64_t minimum, mln_operation* out
) -> mln_status {
  if (!out || *out != MLN_HANDLE_NULL) return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->state != MLN_RENDER_SESSION_STATE_ATTACHED)
      return MLN_STATUS_INVALID_STATE;
  }
  auto operation = std::shared_ptr<OperationObject>{};
  const auto status = register_operation(
    s->operation_source, RENDER_OPERATION_BARRIER, false, {}, out, operation
  );
  if (status != MLN_STATUS_OK) return status;
  auto item = make_ordered_barrier_work(s, minimum, operation);
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      abandon_operation(*out);
      *out = MLN_HANDLE_NULL;
      return MLN_STATUS_INVALID_STATE;
    }
    ++s->barrier_epoch;
    auto& queue =
      s->waiting_update_work.empty() ? s->driver_work : s->waiting_update_work;
    queue.push_back(std::move(item));
    if (s->waiting_update_work.empty()) publish_driver_work_locked(*s);
  }
  return MLN_STATUS_OK;
}

auto render_session_maintenance_start(
  mln_render_session handle, std::uint32_t kind, mln_operation* out
) -> mln_status {
  return enqueue_driver_operation(
    handle, RENDER_OPERATION_MAINTENANCE,
    [kind](mln_render_session_object& s) {
      switch (kind) {
        case 0:
          return render_session_reduce_memory_use(s.self);
        case 1:
          return render_session_clear_data(s.self);
        default:
          return render_session_dump_debug_logs(s.self);
      }
    },
    out
  );
}

auto render_session_detach_start(mln_render_session handle, mln_operation* out)
  -> mln_status {
  const auto s = lease_render_session(handle);
  if (!s || !out || *out != MLN_HANDLE_NULL) return MLN_STATUS_INVALID_ARGUMENT;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->state != MLN_RENDER_SESSION_STATE_ATTACHED)
      return MLN_STATUS_INVALID_STATE;
    if (s->acquired_frame_count != 0) return MLN_STATUS_INVALID_STATE;
  }
  auto operation = std::shared_ptr<OperationObject>{};
  const auto status = register_operation(
    s->operation_source, RENDER_OPERATION_DETACH, false, {}, out, operation
  );
  if (status != MLN_STATUS_OK) return status;
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (
      s->state != MLN_RENDER_SESSION_STATE_ATTACHED ||
      s->acquired_frame_count != 0
    ) {
      abandon_operation(*out);
      *out = MLN_HANDLE_NULL;
      return MLN_STATUS_INVALID_STATE;
    }
    s->state = MLN_RENDER_SESSION_STATE_DETACHING;
    ++s->barrier_epoch;
    ++s->generation;
  }
  enqueue_work(
    s,
    make_driver_work(
      RenderDriverWorkKind::Detach,
      [s, operation]() {
        static_cast<void>(map_set_render_session_publish_callback(s->map, {}));
        const auto status = render_session_detach(s->self);
        {
          const auto lock = std::scoped_lock{s->control_mutex};
          s->state = status == MLN_STATUS_OK
                       ? MLN_RENDER_SESSION_STATE_DETACHED
                       : MLN_RENDER_SESSION_STATE_TARGET_LOST;
          ++s->generation;
        }
        operation->complete(status, {}, {});
      },
      [operation]() {
        operation->complete(MLN_STATUS_TARGET_LOST, "target abandoned", {});
      }
    )
  );
  return MLN_STATUS_OK;
}

auto render_session_abandon(
  mln_render_session handle, mln_render_abandon_result* out
) -> mln_status {
  if (!out || out->size < sizeof(*out)) return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  auto discarded = std::deque<RenderDriverWorkItem>{};
  auto pending_demands = std::deque<PendingFrameDemand>{};
  {
    const auto lock = std::scoped_lock{s->control_mutex};
    if (s->driver_call_in_flight) return MLN_STATUS_BUSY;
    if (
      s->state == MLN_RENDER_SESSION_STATE_DETACHED ||
      s->state == MLN_RENDER_SESSION_STATE_ABANDONED
    )
      return MLN_STATUS_INVALID_STATE;
    s->state = MLN_RENDER_SESSION_STATE_ABANDONED;
    s->attached = false;
    s->target_ready = false;
    s->stop_worker = true;
    discarded.swap(s->driver_work);
    while (!s->waiting_update_work.empty()) {
      discarded.push_back(std::move(s->waiting_update_work.front()));
      s->waiting_update_work.pop_front();
    }
    pending_demands.swap(s->demands);
    ++s->generation;
    s->worker_condition.notify_all();
  }
  static_cast<void>(map_set_render_session_publish_callback(s->map, {}));
  static_cast<void>(map_detach_render_target_session(s->map, s.get()));
  for (auto& item : discarded) {
    auto& callbacks = driver_work_callbacks(item);
    if (callbacks.abandon) callbacks.abandon();
  }
  for (const auto& pending : pending_demands) {
    publish_frame_result(
      s, mln_render_frame_result{
           sizeof(mln_render_frame_result), MLN_RENDER_RESULT_TARGET_NOT_READY,
           pending.demand.token, s->map_update_generation, s->extent_generation,
           0, pending.demand.presentation_time_ns
         }
    );
  }
  auto quarantined = uint32_t{0};
  if (s->renderer.release() != nullptr) ++quarantined;
  if (s->surface.backend.release() != nullptr) ++quarantined;
  if (s->texture.backend.release() != nullptr) ++quarantined;
  s->scheduler.set_repaint_request({});
  s->scheduler.discard();
  s->frame_endpoint.reset();
  s->driver_endpoint.reset();
  *out = mln_render_abandon_result{
    sizeof(*out),
    quarantined == 0 ? MLN_RENDER_ABANDON_DISPOSITION_CLEAN
                     : MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED,
    quarantined, 0
  };
  return MLN_STATUS_OK;
}

auto render_session_set_feature_state_start(
  mln_render_session handle, mln_buffer_view source, mln_buffer_view layer,
  mln_buffer_view feature, mln_buffer_view state, mln_operation* out
) -> mln_status {
  if (
    !validate_string_view(source) || !validate_string_view(layer) ||
    !validate_string_view(feature) || !validate_bytes(state, "feature state")
  )
    return MLN_STATUS_INVALID_ARGUMENT;
  const auto a = string_from_view(source), b = string_from_view(layer);
  const auto c = string_from_view(feature);
  const auto d =
    state.size == 0
      ? std::string{}
      : std::string{static_cast<const char*>(state.data), state.size};
  return enqueue_driver_operation(
    handle, RENDER_OPERATION_FEATURE_STATE_SET,
    [a, b, c, d](mln_render_session_object& s) {
      auto selector = mln_feature_state_selector{
        sizeof(mln_feature_state_selector),
        MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
          MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
        {a.data(), a.size()},
        {b.data(), b.size()},
        {c.data(), c.size()},
        {}
      };
      return render_session_set_feature_state(
        s.self, &selector, {d.data(), d.size()}
      );
    },
    out
  );
}

auto render_session_remove_feature_state_start(
  mln_render_session handle, mln_buffer_view source, mln_buffer_view layer,
  mln_buffer_view feature, mln_buffer_view key, mln_operation* out
) -> mln_status {
  if (
    !validate_string_view(source) || !validate_string_view(layer) ||
    !validate_string_view(feature) || !validate_string_view(key)
  )
    return MLN_STATUS_INVALID_ARGUMENT;
  const auto a = string_from_view(source), b = string_from_view(layer);
  const auto c = string_from_view(feature), d = string_from_view(key);
  return enqueue_driver_operation(
    handle, RENDER_OPERATION_FEATURE_STATE_REMOVE,
    [a, b, c, d](mln_render_session_object& s) {
      auto fields = uint32_t{0};
      if (!b.empty()) fields |= MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
      if (!c.empty()) fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
      if (!d.empty()) fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
      auto selector = mln_feature_state_selector{
        sizeof(mln_feature_state_selector),
        fields,
        {a.data(), a.size()},
        {b.data(), b.size()},
        {c.data(), c.size()},
        {d.data(), d.size()}
      };
      return render_session_remove_feature_state(s.self, &selector);
    },
    out
  );
}

auto render_session_get_feature_state_start(
  mln_render_session handle, mln_buffer_view source, mln_buffer_view layer,
  mln_buffer_view feature, mln_operation* out
) -> mln_status {
  if (
    !validate_string_view(source) || !validate_string_view(layer) ||
    !validate_string_view(feature) || !out || *out != MLN_HANDLE_NULL
  )
    return MLN_STATUS_INVALID_ARGUMENT;
  const auto s = lease_render_session(handle);
  if (!s) return MLN_STATUS_INVALID_ARGUMENT;
  auto operation = std::shared_ptr<OperationObject>{};
  const auto status = register_operation(
    s->operation_source, RENDER_OPERATION_FEATURE_STATE_GET, false, {}, out,
    operation
  );
  if (status != MLN_STATUS_OK) return status;
  const auto a = string_from_view(source), b = string_from_view(layer);
  const auto c = string_from_view(feature);
  enqueue_work(
    s, make_driver_work(
         RenderDriverWorkKind::FeatureState,
         [s, operation, a, b, c]() {
           auto selector = mln_feature_state_selector{
             sizeof(mln_feature_state_selector),
             MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
               MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
             {a.data(), a.size()},
             {b.data(), b.size()},
             {c.data(), c.size()},
             {}
           };
           auto result = mln_buffer{MLN_HANDLE_NULL};
           const auto status =
             render_session_get_feature_state(s->self, &selector, &result);
           if (status != MLN_STATUS_OK) {
             operation->complete(status, {}, {});
             return;
           }
           const auto owned = buffer_table().remove(result);
           if (!owned) {
             operation->complete(
               MLN_STATUS_NATIVE_ERROR, "feature-state result was lost", {}
             );
             return;
           }
           operation->complete(
             MLN_STATUS_OK, {}, std::any{std::move(owned->bytes)}
           );
         },
         [operation]() {
           operation->complete(MLN_STATUS_TARGET_LOST, "target abandoned", {});
         }
       )
  );
  return MLN_STATUS_OK;
}

auto render_session_get_feature_state_take_result(
  mln_operation operation, mln_buffer* out
) -> mln_status {
  if (!out || *out != MLN_HANDLE_NULL) return MLN_STATUS_INVALID_ARGUMENT;
  return take_operation_result<std::string>(
    operation, RENDER_OPERATION_FEATURE_STATE_GET,
    [out](std::string& value) { return create_buffer(std::move(value), out); }
  );
}

}  // namespace mln::core
