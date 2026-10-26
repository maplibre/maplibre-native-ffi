#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__ANDROID__) && defined(MLN_RENDER_BACKEND_OPENGL)
#include <sys/system_properties.h>
#endif

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/renderer_backend.hpp>
#include <mln/map/map.hpp>
#include <mln/renderer/query.hpp>
#include <mln/renderer/renderer.hpp>
#include <mln/renderer/update_parameters.hpp>
#include <mln/style/filter.hpp>
#include <mln/style/source_impl.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/geo.hpp>
#include <mln/util/geojson.hpp>
#include <mln/util/logging.hpp>
#include <mln/util/size.hpp>
#include <mln/util/string.hpp>

#if defined(MLN_RENDER_BACKEND_VULKAN)
#include <mln/vulkan/renderable_resource.hpp>
#endif

#include "c_api/autorelease_pool.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "render/render_session_common.hpp"
#include "runtime/runtime.hpp"
#include "style/style_value.hpp"
#include "testing/sync_point.hpp"

namespace mln::core {

auto logical_extent_physical_size(
  mln_logical_extent extent, uint32_t* out_width, uint32_t* out_height
) -> mln_status {
  if (out_width == nullptr || out_height == nullptr) {
    set_thread_error("out_width and out_height must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    extent, "extent dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto physical_status = validate_physical_size(
    extent.width, extent.height, extent.scale_factor,
    "scaled extent dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  *out_width = physical_dimension(extent.width, extent.scale_factor);
  *out_height = physical_dimension(extent.height, extent.scale_factor);
  return MLN_STATUS_OK;
}

auto opengl_supported_context_provider_mask() noexcept -> uint32_t {
#if defined(MLN_RENDER_BACKEND_OPENGL) && defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  return MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL;
#elif defined(MLN_RENDER_BACKEND_OPENGL) && defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  return MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL;
#elif defined(MLN_RENDER_BACKEND_OPENGL) && \
  defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  return MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL;
#else
  return 0;
#endif
}

// The instance and queue are optional: no session kind needs the instance, and
// a null queue means the device's default queue.
auto validate_webgpu_context(const mln_web_gpu_context_descriptor& context)
  -> mln_status {
  if (context.device == nullptr) {
    set_thread_error("WebGPU device must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_opengl_context(
  const mln_open_gl_context_descriptor& context, bool require_supported_provider
) -> mln_status {
  if (
    context.ownership != MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED &&
    context.ownership != MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED
  ) {
    set_thread_error("mln_open_gl_context_descriptor.ownership is unknown");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto dedicated =
    context.ownership == MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;

  if (context.platform == MLN_OPEN_GL_CONTEXT_PLATFORM_WGL) {
    if (
      require_supported_provider && (opengl_supported_context_provider_mask() &
                                     MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL) == 0
    ) {
      set_thread_error("OpenGL WGL context provider is not supported");
      return MLN_STATUS_UNSUPPORTED;
    }
    if (context.data.wgl.device_context == nullptr) {
      set_thread_error("WGL device_context must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (dedicated) {
      if (context.data.wgl.share_context != nullptr) {
        set_thread_error(
          "a dedicated WGL context joins no share group, so "
          "share_context must "
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

  if (context.platform == MLN_OPEN_GL_CONTEXT_PLATFORM_EGL) {
    if (
      require_supported_provider && (opengl_supported_context_provider_mask() &
                                     MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL) == 0
    ) {
      set_thread_error("OpenGL EGL context provider is not supported");
      return MLN_STATUS_UNSUPPORTED;
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
          "a dedicated EGL context joins no share group, so "
          "share_context must "
          "be null"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (
        context.data.egl.client_api != MLN_OPEN_GL_CLIENT_API_GL &&
        context.data.egl.client_api != MLN_OPEN_GL_CLIENT_API_GLES
      ) {
        set_thread_error(
          "a dedicated EGL context has no share context to take "
          "its client API "
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

  if (context.platform == MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL) {
    if (
      require_supported_provider &&
      (opengl_supported_context_provider_mask() &
       MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL) == 0
    ) {
      set_thread_error("OpenGL WebGL context provider is not supported");
      return MLN_STATUS_UNSUPPORTED;
    }
    if (context.data.web_gl.kind == MLN_WEB_GL_CONTEXT_EXISTING) {
      if (dedicated) {
        set_thread_error("an existing WebGL context must use shared ownership");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (context.data.web_gl.context <= 0) {
        set_thread_error("WebGL context handle must be positive");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      return MLN_STATUS_OK;
    }
    if (context.data.web_gl.kind == MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS) {
      if (!dedicated) {
        set_thread_error(
          "a transferred WebGL canvas must use dedicated ownership"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (
        context.data.web_gl.context != 0 ||
        context.data.web_gl.canvas_selector.data == nullptr ||
        context.data.web_gl.canvas_selector.size == 0
      ) {
        set_thread_error(
          "a transferred WebGL canvas requires a selector and "
          "no context handle"
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
  const mln_open_gl_context_descriptor& lhs,
  const mln_open_gl_context_descriptor& rhs, OpenGLContextMatch strictness
) -> bool {
  if (lhs.platform != rhs.platform) {
    return false;
  }
  switch (lhs.platform) {
    case MLN_OPEN_GL_CONTEXT_PLATFORM_WGL:
      if (lhs.ownership != rhs.ownership) {
        return false;
      }
      // A dedicated context names no share context, so the device context it
      // was created from is what identifies it under either strictness.
      if (
        (strictness == OpenGLContextMatch::Exact ||
         lhs.ownership == MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED) &&
        lhs.data.wgl.device_context != rhs.data.wgl.device_context
      ) {
        return false;
      }
      return lhs.data.wgl.share_context == rhs.data.wgl.share_context;
    case MLN_OPEN_GL_CONTEXT_PLATFORM_EGL:
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
        lhs.ownership == MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED &&
        lhs.data.egl.client_api != rhs.data.egl.client_api
      ) {
        return false;
      }
      return lhs.data.egl.display == rhs.data.egl.display &&
             lhs.data.egl.config == rhs.data.egl.config &&
             lhs.data.egl.share_context == rhs.data.egl.share_context;
    case MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL:
      if (lhs.data.web_gl.kind != rhs.data.web_gl.kind) {
        return false;
      }
      if (lhs.data.web_gl.kind == MLN_WEB_GL_CONTEXT_EXISTING) {
        return lhs.data.web_gl.context == rhs.data.web_gl.context;
      }
      return lhs.data.web_gl.canvas_selector.size ==
               rhs.data.web_gl.canvas_selector.size &&
             std::equal(
               static_cast<const std::byte*>(
                 lhs.data.web_gl.canvas_selector.data
               ),
               static_cast<const std::byte*>(
                 lhs.data.web_gl.canvas_selector.data
               ) +
                 lhs.data.web_gl.canvas_selector.size,
               static_cast<const std::byte*>(
                 rhs.data.web_gl.canvas_selector.data
               )
             );
    case MLN_OPEN_GL_CONTEXT_PLATFORM_UNSPECIFIED:
      break;
  }
  return false;
}

}  // namespace mln::core

namespace {

#if defined(__ANDROID__) && defined(MLN_RENDER_BACKEND_OPENGL)
// Same property check MapLibre Android uses before it enables the Goldfish
// drawable-clear workaround on the emulator OpenGL translation layer.
auto android_system_property(const char* key) -> std::string {
  char value[PROP_VALUE_MAX + 1] = {};
  __system_property_get(key, value);
  return value;
}

auto android_opengl_emulator() -> bool {
  return android_system_property("ro.kernel.qemu") == "1" ||
         android_system_property("ro.boot.qemu") == "1" ||
         android_system_property("ro.hardware.egl") == "emulation";
}
#endif

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

// Null once the session has released its target, which is what every caller's
// null check is looking for.
auto renderer_backend(mln_render_session_object* session)
  -> mln::gfx::RendererBackend* {
  if (session->kind == mln::core::RenderSessionKind::Surface) {
    return session->surface.backend == nullptr
             ? nullptr
             : &session->surface.backend->renderer_backend();
  }
  return session->texture.backend == nullptr
           ? nullptr
           : session->texture.backend->renderer_backend();
}

auto validate_renderer_backend(
  mln_render_session_object* session, mln::gfx::RendererBackend*& out_backend
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

auto make_string_vector(std::span<const mln_buffer_view> strings)
  -> std::vector<std::string> {
  auto result = std::vector<std::string>{};
  result.reserve(strings.size());
  for (const auto string : strings) {
    result.emplace_back(string_from_view(string));
  }
  return result;
}

// Draws without handing the frame to the host target. The previous setting is
// restored rather than cleared, so an inner warmup render nests inside a demand
// that already asked for no presentation.
class DiscardedPresent {
 public:
  explicit DiscardedPresent(bool discard)
      : previous_(mln::core::discard_renderable_present) {
    mln::core::discard_renderable_present =
      mln::core::discard_renderable_present || discard;
  }
  DiscardedPresent(const DiscardedPresent&) = delete;
  auto operator=(const DiscardedPresent&) -> DiscardedPresent& = delete;
  DiscardedPresent(DiscardedPresent&&) = delete;
  auto operator=(DiscardedPresent&&) -> DiscardedPresent& = delete;
  ~DiscardedPresent() { mln::core::discard_renderable_present = previous_; }

 private:
  bool previous_;
};

// A warmup render, which also keeps its frame callbacks from reaching the map.
class UnpresentedRender {
 public:
  explicit UnpresentedRender(mln::core::SessionFrameObserver& observer)
      : observer_(observer), present_(true) {
    observer_.suppress_frame_callbacks(true);
  }
  UnpresentedRender(const UnpresentedRender&) = delete;
  auto operator=(const UnpresentedRender&) -> UnpresentedRender& = delete;
  UnpresentedRender(UnpresentedRender&&) = delete;
  auto operator=(UnpresentedRender&&) -> UnpresentedRender& = delete;
  ~UnpresentedRender() { observer_.suppress_frame_callbacks(false); }

 private:
  mln::core::SessionFrameObserver& observer_;
  DiscardedPresent present_;
};

void reset_pushed_feature_state(mln_render_session_object& session) {
  session.rendered_source_ids.clear();
  session.rendered_sources.reset();
  session.applied_feature_state = {};
  session.pushed_feature_state.reset();
}

auto prepare_surface_frame(mln_render_session_object& session, bool& out_ready)
  -> mln_status {
  out_ready = true;
  if (session.kind != mln::core::RenderSessionKind::Surface) {
    return MLN_STATUS_OK;
  }
  try {
    return session.surface.backend->prepare_frame(out_ready);
  } catch (const std::exception& exception) {
    set_native_stage_error("preparing surface frame", exception);
    return MLN_STATUS_NATIVE_ERROR;
  }
}

auto to_rendered_query_options(
  const mln_rendered_feature_query_options* options
) -> std::optional<mln::RenderedQueryOptions> {
  auto layer_ids = std::optional<std::vector<std::string>>{};
  auto filter = std::optional<mln::style::Filter>{};
  if (options == nullptr) {
    return mln::RenderedQueryOptions{};
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
  if (options->filter.size != 0) {
    auto converted_filter = mln::core::to_native_style_filter(&options->filter);
    if (!converted_filter) {
      return std::nullopt;
    }
    filter = std::move(*converted_filter);
  }
  return mln::RenderedQueryOptions{std::move(layer_ids), std::move(filter)};
}

auto to_source_query_options(const mln_source_feature_query_options* options)
  -> std::optional<mln::SourceQueryOptions> {
  auto source_layer_ids = std::optional<std::vector<std::string>>{};
  auto filter = std::optional<mln::style::Filter>{};
  if (options == nullptr) {
    return mln::SourceQueryOptions{};
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
  if (options->filter.size != 0) {
    auto converted_filter = mln::core::to_native_style_filter(&options->filter);
    if (!converted_filter) {
      return std::nullopt;
    }
    filter = std::move(*converted_filter);
  }
  return mln::SourceQueryOptions{
    std::move(source_layer_ids), std::move(filter)
  };
}

auto to_screen_line_string(
  const mln_screen_line_string& line_string, mln::ScreenLineString& out_line
) -> bool {
  if (line_string.point_count == 0) {
    mln::core::set_thread_error("query line string must contain points");
    return false;
  }
  if (line_string.points == nullptr) {
    mln::core::set_thread_error("query line string points must not be null");
    return false;
  }
  auto result = mln::ScreenLineString{};
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
) -> std::optional<mln::ScreenBox> {
  const auto view_width = static_cast<double>(width);
  const auto view_height = static_cast<double>(height);
  const auto min_x = std::min(box.min.x, box.max.x);
  const auto min_y = std::min(box.min.y, box.max.y);
  const auto max_x = std::max(box.min.x, box.max.x);
  const auto max_y = std::max(box.min.y, box.max.y);
  if (min_x > view_width || min_y > view_height || max_x < 0.0 || max_y < 0.0) {
    return std::nullopt;
  }
  return mln::ScreenBox{
    {std::max(min_x, 0.0), std::max(min_y, 0.0)},
    {std::min(max_x, view_width), std::min(max_y, view_height)}
  };
}

auto serialize_geojson_feature(const mln::Feature& feature) -> std::string {
  return mln::core::serialize_geojson(
    mln::GeoJSON{static_cast<const mln::GeoJSONFeature&>(feature)}
  );
}

auto create_rendered_feature_result(
  const std::vector<mln::Feature>& features,
  std::shared_ptr<const mln::core::RenderedFeatureList>& out_result
) -> mln_status {
  auto list = std::make_shared<mln::core::RenderedFeatureList>();
  list->features.reserve(features.size());
  for (const auto& feature : features) {
    auto record = mln::core::RenderedFeatureRecord{};
    record.feature = serialize_geojson_feature(feature);
    record.layer_id = feature.layer;
    record.source_id = feature.source;
    record.source_layer_id = feature.sourceLayer;
    if (!feature.state.empty()) {
      record.state = mln::core::serialize_json_value(mln::Value{feature.state});
    }
    list->features.push_back(std::move(record));
  }
  out_result = std::move(list);
  return MLN_STATUS_OK;
}

auto create_source_feature_result(
  const std::vector<mln::Feature>& features,
  std::shared_ptr<const mln::core::SourceFeatureList>& out_result
) -> mln_status {
  auto list = std::make_shared<mln::core::SourceFeatureList>();
  list->features.reserve(features.size());
  for (const auto& feature : features) {
    auto record = mln::core::SourceFeatureRecord{};
    record.feature = serialize_geojson_feature(feature);
    record.source_layer_id = feature.sourceLayer;
    list->features.push_back(std::move(record));
  }
  out_result = std::move(list);
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

auto to_feature_extension_arguments(mln_buffer_view arguments)
  -> std::optional<std::optional<std::map<std::string, mln::Value>>> {
  if (arguments.size == 0) {
    return std::optional<std::map<std::string, mln::Value>>{std::nullopt};
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
  auto result = std::map<std::string, mln::Value>{};
  for (const auto& [key, value] : *object) {
    result.emplace(key, value);
  }
  return std::optional<std::map<std::string, mln::Value>>{std::move(result)};
}

auto serialize_feature_extension_result(const mln::FeatureExtensionValue& value)
  -> std::string {
  if (value.is<mln::Value>()) {
    return mln::core::serialize_json_value(value.get<mln::Value>());
  }
  return mln::core::serialize_feature_collection(
    value.get<mln::FeatureCollection>()
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
  mln::Log::Warning(
    mln::Event::Render,
    "render target scale_factor " + mln::util::toString(scale_factor) +
      " differs from the map scale_factor " +
      mln::util::toString(creation_scale_factor) +
      "; the map value is fixed at creation and still selects sprites, "
      "glyphs, "
      "and raster tiles, so styled imagery will not match the rendered "
      "geometry. Create the map with the scale factor you intend to render "
      "at."
  );
}

}  // namespace

namespace mln::core {

namespace {
auto session_teardown_lane() -> RetirementLane& {
  static auto* lane = new RetirementLane{};
  return *lane;
}

auto session_idle_locked(const mln_render_session_object& session) noexcept
  -> bool {
  return !session.driver_call_in_flight && session.active_views == 0;
}

// Runs on the lane. A retirement that finds its session busy parks there, in
// order, and returns, so the lane moves on to other sessions.
auto park_retirement_while_busy(
  mln_render_session_object& session, RetirementTask& task
) noexcept -> bool {
  const auto lock = std::scoped_lock{session.control_mutex};
  if (session_idle_locked(session)) return false;
  auto** tail = &session.parked_retirements;
  while (*tail != nullptr) tail = &(*tail)->next;
  *tail = &task;
  return true;
}

// Every transition that can make a session idle calls this under the control
// lock and passes the result to resubmit_parked_retirements after releasing
// it.
auto take_parked_retirements_locked(mln_render_session_object& session) noexcept
  -> RetirementTask* {
  if (!session_idle_locked(session)) return nullptr;
  return std::exchange(session.parked_retirements, nullptr);
}

auto resubmit_parked_retirements(RetirementTask* task) noexcept -> void {
  while (task != nullptr) {
    // The lane may run and destroy the task's owner once it is submitted.
    auto* next = std::exchange(task->next, nullptr);
    session_teardown_lane().submit(*task);
    task = next;
  }
}
// What abandon does with the graphics objects it takes from a session.
enum class AbandonedGraphics : std::uint8_t {
  // Destroys each one whose destruction is safe on the abandoning thread.
  DestroyWhenSafe,
  // Keeps every one until the process exits, making no graphics call.
  Keep,
};

auto abandon_render_session(
  const std::shared_ptr<mln_render_session_object>& live,
  mln_render_abandon_result* out_result,
  AbandonedGraphics graphics = AbandonedGraphics::DestroyWhenSafe
) -> mln_status;
auto destroy_render_session(
  const std::shared_ptr<mln_render_session_object>& live
) -> mln_status;
}  // namespace

auto register_render_session(std::shared_ptr<mln_render_session_object> session)
  -> mln_render_session {
  return handle_table<mln_render_session_object>().insert(std::move(session));
}

void RenderSessionScheduler::schedule(std::function<void()>&& task) {
  auto notify_work = false;
  {
    const auto lock = std::scoped_lock{mutex_};
    notify_work = queue_.empty() && !draining_;
    queue_.push_back(std::move(task));
  }
  if (notify_work) notify_work_available();
}

void RenderSessionScheduler::schedule(
  const mln::util::SimpleIdentity, std::function<void()>&& task
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
    {
      const auto lock = std::scoped_lock{mutex_};
      if (queue_.empty()) {
        return;
      }
      drain_batch_.swap(queue_);
    }
    // Run outside the lock: a task may schedule more work.
    for (auto& task : drain_batch_) {
      if (task) {
        task();
      }
    }
    drain_batch_.clear();
  }
}

RenderSessionScheduler::DrainGuard::~DrainGuard() {
  scheduler_.drain_batch_.clear();
  auto notify_work = false;
  {
    const auto lock = std::scoped_lock{scheduler_.mutex_};
    if (scheduler_.draining_) {
      scheduler_.draining_ = false;
      notify_work = !scheduler_.queue_.empty();
    }
  }
  if (notify_work) scheduler_.notify_work_available();
}

auto RenderSessionScheduler::discard() -> void {
  auto batch = std::vector<std::function<void()>>{};
  const auto lock = std::scoped_lock{mutex_};
  batch.swap(queue_);
}

auto RenderSessionScheduler::set_work_available_callback(
  std::function<void()> work_available
) -> void {
  auto lock = std::unique_lock{hook_mutex_, std::try_to_lock};
  if (!lock.owns_lock()) {
    mln::testing::hit(mln::testing::SyncPoint::RenderSchedulerHookWaits);
    lock.lock();
  }
  work_available_ = std::move(work_available);
}

auto RenderSessionScheduler::notify_work_available() -> void {
  const auto lock = std::scoped_lock{hook_mutex_};
  if (work_available_) work_available_();
}

namespace {

auto finish_driver_work(
  const std::shared_ptr<OperationObject>& operation,
  const RenderDriverCallable& callable,
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void {
  try {
    const auto status = callable(*session);
    operation->complete(
      status,
      status == MLN_STATUS_OK ? std::string{}
                              : std::string{thread_last_error_message()},
      {}
    );
  } catch (const std::exception& exception) {
    operation->complete(MLN_STATUS_NATIVE_ERROR, exception.what(), {});
  } catch (...) {
    operation->complete(
      MLN_STATUS_NATIVE_ERROR, "render driver work failed", {}
    );
  }
}

// Whether a texture ring has no slot left to render into. A surface has no
// ring and renders into its target directly.
auto every_slot_quarantined_locked(const mln_render_session_object& session)
  -> bool {
  const auto& slots = session.texture.slots;
  return !slots.empty() &&
         std::all_of(slots.begin(), slots.end(), [](const auto& slot) {
           return slot.quarantined;
         });
}

auto any_slot_quarantined_locked(const mln_render_session_object& session)
  -> bool {
  const auto& slots = session.texture.slots;
  return std::any_of(slots.begin(), slots.end(), [](const auto& slot) {
    return slot.quarantined;
  });
}

// Retires every published frame that no host holds, so none can be acquired
// after the target it was rendered into changes.
void invalidate_unacquired_texture_frames_locked(
  mln_render_session_object& session
) {
  if (session.kind != RenderSessionKind::Texture) return;
  for (auto& slot : session.texture.slots) {
    if (!slot.acquired) {
      slot.available = false;
      slot.backend_metadata.reset();
    }
  }
}

// The host wakes that a section holding control_mutex owes. A wake callback may
// call back into the session, which would relock control_mutex, so the section
// records its wakes here and they run when this object is destroyed. Declare it
// before the lock so the lock is released first. The coalescing flags are set
// under the lock as before, and every recorded wake still runs, so none is
// lost.
class DeferredWakes final {
 public:
  DeferredWakes() = default;
  DeferredWakes(const DeferredWakes&) = delete;
  DeferredWakes(DeferredWakes&&) = delete;
  auto operator=(const DeferredWakes&) -> DeferredWakes& = delete;
  auto operator=(DeferredWakes&&) -> DeferredWakes& = delete;
  ~DeferredWakes() {
    if (frame_ != nullptr) {
      mln::testing::hit(mln::testing::SyncPoint::RenderFrameWakeDeferred);
      frame_->notify();
    }
    if (driver_ != nullptr) driver_->notify();
  }

  auto frame(const std::shared_ptr<Wake>& wake) noexcept -> void {
    frame_ = wake;
  }
  auto driver(const std::shared_ptr<Wake>& wake) noexcept -> void {
    driver_ = wake;
  }

 private:
  std::shared_ptr<Wake> frame_;
  std::shared_ptr<Wake> driver_;
};

// Called once the session has moved its wakes out. Another thread can still
// hold a copy that it took under the lock and has yet to fire; closing makes
// that late notify do nothing, so no wake starts after the session lets go.
auto close_wakes(
  const std::shared_ptr<Wake>& frame, const std::shared_ptr<Wake>& driver
) noexcept -> void {
  if (frame != nullptr) frame->close();
  if (driver != nullptr) driver->close();
}

// Whether the driver can take the item at the front of its queue now. A
// pending driver result holds the queue until it is ready.
// Whether the work at the front of the queue renders a frame while the GPU has
// as many unfinished frames as the texture ring has slots.
auto frame_waits_for_gpu_locked(
  const mln_render_session_object& session
) noexcept -> bool {
  const auto& texture = session.texture;
  return !session.driver_work.empty() &&
         session.driver_work.front().renders_frame && !texture.slots.empty() &&
         texture.gpu_frames_in_flight >= texture.slots.size();
}

auto driver_work_runnable_locked(
  const mln_render_session_object& session
) noexcept -> bool {
  if (session.driver_work.empty()) return false;
  if (frame_waits_for_gpu_locked(session)) return false;
  const auto& ready = session.driver_work.front().ready;
  return !ready || ready();
}

// Wakes the driver when it can take work. Work queued behind a pending driver
// result wakes nothing until resume_driver_work() reports the result ready.
// A frame that the frames in flight hold back wakes a caller driver once,
// since only a service asks the backend to report their end.
auto publish_driver_work_locked(
  mln_render_session_object& session, DeferredWakes& wakes
) noexcept -> void {
  if (
    !driver_work_runnable_locked(session) &&
    (!frame_waits_for_gpu_locked(session) || session.texture.gpu_wait_pending)
  )
    return;
  if (session.capabilities.driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    session.worker_condition.notify_one();
  } else if (session.driver_wake != nullptr && !session.driver_wake_pending) {
    session.driver_wake_pending = true;
    wakes.driver(session.driver_wake);
  }
}

// Queues work in driver order.
auto push_driver_work_locked(
  mln_render_session_object& session, RenderDriverWork work,
  DeferredWakes& wakes
) noexcept -> void {
  auto& queue = session.driver_work;
  queue.push_back(std::move(work));
  publish_driver_work_locked(session, wakes);
}

auto run_frame_demand(
  const std::shared_ptr<mln_render_session_object>& session,
  const std::shared_ptr<const mln_frame_demand>& expected
) noexcept -> void;

// Queues work for a demand, defaulting to the session's front demand. If
// allocation fails, later driver work or detach gives the demand a result.
auto queue_frame_demand_work_locked(
  mln_render_session_object& session, DeferredWakes& wakes,
  const std::shared_ptr<const mln_frame_demand>& demand = {}
) noexcept -> bool {
  const auto expected = demand ? demand : session.demands.front();
  auto& queue = session.driver_work;
  try {
    queue.push_back(
      RenderDriverWork{
        .execute = [owner = session.shared_from_this(),
                    expected]() { run_frame_demand(owner, expected); },
        .abandon = {},
        .ready = {},
        .renders_frame = true,
      }
    );
  } catch (...) {
    return false;
  }
  publish_driver_work_locked(session, wakes);
  return true;
}

// Runs the front demand again. A full ring parks a demand without a work item,
// so a released frame that frees a slot runs it, as does a quarantined ring
// once the demand ahead of it ended.
auto resume_parked_demand_locked(
  mln_render_session_object& session, DeferredWakes& wakes
) noexcept -> void {
  if (
    session.demands.empty() ||
    (session.state != MLN_RENDER_SESSION_STATE_ATTACHED &&
     session.state != MLN_RENDER_SESSION_STATE_DETACHING)
  )
    return;
  static_cast<void>(queue_frame_demand_work_locked(session, wakes));
}

// Moves every demand that waits for a map update back to the front of the
// queue, ahead of the later demands there, in the order they began waiting.
// Returns how many moved.
auto requeue_update_waiting_demands_locked(mln_render_session_object& session)
  -> std::size_t {
  auto& waiting = session.update_waiting_demands;
  const auto count = waiting.size();
  while (!waiting.empty()) {
    session.demands.push_front(waiting.back());
    waiting.pop_back();
  }
  return count;
}

// Submits the demand that renders the map's pending still image, unless the
// session cannot render now, or already rendered a frame that completes the
// image and the map has yet to say so. While a demand submitted for that image
// is outstanding, its result submits the next instead. Called wherever the
// session can render again: the request itself, attachment, a barrier's
// completion, and wherever waiting demands resume. A demand that cannot be
// allocated leaves the image for the next of those.
auto arm_still_image_demand_locked(
  mln_render_session_object& session, DeferredWakes& wakes
) noexcept -> void {
  if (
    session.state != MLN_RENDER_SESSION_STATE_ATTACHED ||
    session.pending_frame_barriers != 0 ||
    every_slot_quarantined_locked(session)
  )
    return;
  const auto image = map_still_image_generation(*session.map_object);
  if (image == 0 || session.rendered_update_generation >= image) return;
  if (
    session.still_image_demands != 0 &&
    session.still_image_demand_generation == image
  ) {
    session.still_image_rearm = true;
    return;
  }
  try {
    session.demands.push_back(
      std::make_shared<const mln_frame_demand>(mln_frame_demand{
        .size = sizeof(mln_frame_demand),
        .flags = MLN_FRAME_DEMAND_PRESENT | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE,
        .token = MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE,
      })
    );
  } catch (...) {
    return;
  }
  if (!queue_frame_demand_work_locked(session, wakes, session.demands.back())) {
    session.demands.pop_back();
    return;
  }
  ++session.still_image_demands;
  session.still_image_demand_generation = image;
}

// Runs every demand that waits for a map update again. Called wherever an
// if-needed demand can become renderable without a newer map update reaching
// it first: the update itself, a target replacement, and an applied resize.
auto resume_update_waiting_demands_locked(
  mln_render_session_object& session, DeferredWakes& wakes
) -> void {
  const auto count = requeue_update_waiting_demands_locked(session);
  for (auto index = std::size_t{0}; index < count; ++index) {
    if (!queue_frame_demand_work_locked(session, wakes, session.demands[index]))
      return;
  }
  arm_still_image_demand_locked(session, wakes);
}

// Publishes `disposition` for a demand that ends without starting.
auto finish_demand_locked(
  mln_render_session_object& session, const mln_frame_demand& pending,
  mln_render_result disposition, DeferredWakes& wakes
) noexcept -> void;

// Gives every demand that waits for a map update the terminal `disposition`,
// in acceptance order.
auto finish_waiting_demands_locked(
  mln_render_session_object& session, mln_render_result disposition,
  DeferredWakes& wakes
) noexcept -> void;

// Gives every demand that has yet to start the terminal `disposition`: those
// that wait for a map update, then the queued ones, each in acceptance order.
auto finish_demands_locked(
  mln_render_session_object& session, mln_render_result disposition,
  DeferredWakes& wakes
) noexcept -> void;

// Quarantines a slot whose texture the host's GPU may still read, so the ring
// never renders into it again. Once no slot can take a frame, a demand that
// waits for a map update could never render one and ends now. A queued demand
// ends when the driver reaches it, behind the work accepted before it, so the
// one a full ring parked runs again.
auto quarantine_slot_locked(
  mln_render_session_object& session, RenderTextureSlot& slot,
  DeferredWakes& wakes
) noexcept -> void {
  slot.quarantined = true;
  if (
    session.state != MLN_RENDER_SESSION_STATE_ATTACHED ||
    !every_slot_quarantined_locked(session)
  )
    return;
  finish_waiting_demands_locked(
    session, MLN_RENDER_RESULT_TARGET_NOT_READY, wakes
  );
  resume_parked_demand_locked(session, wakes);
}

// Releases the core worker after stop_worker is set. The teardown lane reaps a
// retired session's worker, and a failed attach reaps its own on the attaching
// thread. On the browser main thread, a worker whose Web Worker has not started
// yet starts only after that thread yields, so a join there would never
// return. The worker is detached instead; it holds its own session reference
// and finds nothing left to run.
auto reap_core_worker(mln_render_session_object& session) -> void {
  if (session.join_worker) {
    session.join_worker();
  } else if (session.worker.joinable()) {
    if (on_browser_main_thread())
      session.worker.detach();
    else
      session.worker.join();
  }
}

auto detach_disposed_session(
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void;

auto run_core_worker(
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void {
  while (true) {
    auto work = RenderDriverWork{};
    {
      auto lock = std::unique_lock{session->control_mutex};
      // An abandon waiting for the previous call to end holds the next one
      // back, so the call it waited out is the last this worker makes.
      session->worker_condition.wait(lock, [&]() noexcept {
        return session->abandon_waiters == 0 &&
               (session->stop_worker || session->disposal_detach ||
                driver_work_runnable_locked(*session));
      });
      if (session->disposal_detach) {
        lock.unlock();
        detach_disposed_session(session);
        return;
      }
      if (
        session->views_invalidated || session->disposal_requested ||
        !driver_work_runnable_locked(*session)
      )
        return;
      work = std::move(session->driver_work.front());
      session->driver_work.pop_front();
      session->driver_call_in_flight = true;
      session->driver_call_thread = current_owner_thread();
    }
    try {
      auto execute = [&]() -> mln_status {
        if (work.execute) work.execute();
        return MLN_STATUS_OK;
      };
      static_cast<void>(mln::c_api::with_autorelease_pool(execute));
    } catch (...) {
      if (work.abandon) work.abandon();
    }
    mln::testing::hit(mln::testing::SyncPoint::RenderDriverExited);
    auto* parked = static_cast<RetirementTask*>(nullptr);
    {
      const auto lock = std::scoped_lock{session->control_mutex};
      session->driver_call_in_flight = false;
      session->driver_call_thread.reset();
      parked = take_parked_retirements_locked(*session);
      session->worker_condition.notify_all();
    }
    resubmit_parked_retirements(parked);
  }
}

auto service_scheduler_work(
  const std::shared_ptr<mln_render_session_object>& session,
  std::shared_ptr<const mln_frame_demand> expected
) noexcept -> void;

using RenderDriverWorkFactory = std::function<RenderDriverWork(
  const std::shared_ptr<mln_render_session_object>&,
  const std::shared_ptr<OperationObject>&
)>;

// What a submission requires of the session beyond attachment, checked in
// the section that queues it, and what queueing it changes.
struct RenderDriverAdmission {
  // Checks the session without changing it.
  std::function<mln_status(const mln_render_session_object&)> check;
  // Records the submission once its work is queued.
  std::function<void(mln_render_session_object&)> commit;
};

// Shared body of the driver submissions: check the session, register the
// completion, and hand one work item to the selected driver. An empty
// `deliver` takes the plain committed completion, and an empty `admit` admits
// every submission to an attached session.
auto submit_driver_work(
  mln_render_session session, const mln_completion* completion,
  CompletionOperation::Delivery deliver,
  const RenderDriverWorkFactory& make_work,
  const RenderDriverAdmission& admit = {},
  std::shared_ptr<Completion> checked = {}
) -> mln_status {
  auto async = CompletionOperation{};
  const auto registered =
    checked
      ? create_completion_operation(
          std::move(checked), std::move(deliver), async
        )
      : create_completion_operation(completion, std::move(deliver), async);
  if (registered != MLN_STATUS_OK) return registered;
  auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
    if (admit.check) {
      const auto admitted = admit.check(*live);
      if (admitted != MLN_STATUS_OK) {
        return admitted;
      }
    }
    // Accepted before the driver can take the work, so the driver, rather
    // than this thread, delivers a completion that finishes early.
    auto work = make_work(live, async.operation);
    async.completion->accept();
    push_driver_work_locked(*live, std::move(work), wakes);
    if (admit.commit) admit.commit(*live);
  }
  return MLN_STATUS_OK;
}

// The delivery and work of a driver submission that completes without a
// value.
auto valueless_driver_work(
  RenderDriverCallable work, const ValuelessCompletion& valueless
) {
  return std::pair{
    CompletionOperation::Delivery{
      [valueless](
        const std::shared_ptr<Completion>& state, mln_status status,
        std::string diagnostic, std::any
      ) { valueless.complete(state, status, std::move(diagnostic)); }
    },
    RenderDriverWorkFactory{
      [work = std::move(work)](
        const std::shared_ptr<mln_render_session_object>& live,
        const std::shared_ptr<OperationObject>& operation
      ) {
        return RenderDriverWork{
          [live, operation, work]() {
            finish_driver_work(operation, work, live);
          },
          [operation]() {
            operation->complete(
              MLN_STATUS_TARGET_LOST, "render target was abandoned", {}
            );
          }
        };
      }
    }
  };
}

auto abandon_driver_result(const std::shared_ptr<OperationObject>& operation)
  -> std::function<void()> {
  return [operation]() {
    operation->complete(
      MLN_STATUS_TARGET_LOST, "render target was abandoned", std::any{}
    );
  };
}

// Runs a driver result's work and completes its operation with what the work
// reports. Work that leaves a PendingDriverResult puts the rest of the result
// at the front of the driver's queue instead, where it holds the work behind
// it until it is ready. The driver popped the running work from that front,
// so the result keeps its place in the accepted order. No abandon can empty
// the queue first: abandon waits out or refuses a driver call in flight.
auto run_driver_result(
  const std::shared_ptr<mln_render_session_object>& live,
  const std::shared_ptr<OperationObject>& operation,
  const std::function<mln_status(std::any&)>& work
) -> void {
  try {
    auto result = std::any{};
    const auto status = work(result);
    auto* pending = std::any_cast<PendingDriverResult>(&result);
    if (status == MLN_STATUS_OK && pending != nullptr) {
      auto rest = RenderDriverWork{
        [live, operation, finish = std::move(pending->finish)]() {
          run_driver_result(live, operation, finish);
        },
        abandon_driver_result(operation), std::move(pending->ready)
      };
      auto wakes = DeferredWakes{};
      const auto lock = std::scoped_lock{live->control_mutex};
      live->driver_work.push_front(std::move(rest));
      // The result may have become ready before it was queued, when its
      // resume_driver_work() found nothing to wake.
      publish_driver_work_locked(*live, wakes);
      return;
    }
    operation->complete(
      status,
      status == MLN_STATUS_OK ? std::string{}
                              : std::string{thread_last_error_message()},
      std::move(result)
    );
  } catch (const std::exception& exception) {
    operation->complete(MLN_STATUS_NATIVE_ERROR, exception.what(), std::any{});
  }
}

}  // namespace

void GpuWorkDone::run() const noexcept {
  const auto live = session.lock();
  if (live == nullptr) return;
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{live->control_mutex};
  // The wait began with the driver held, so every frame in flight preceded it.
  live->texture.gpu_frames_in_flight = 0;
  live->texture.gpu_wait_pending = false;
  publish_driver_work_locked(*live, wakes);
}

auto lease_render_session(mln_render_session session)
  -> std::shared_ptr<mln_render_session_object> {
  auto live = handle_table<mln_render_session_object>().lease(session);
  if (live != nullptr && live->disposal_requested.load()) {
    static_cast<void>(report_handle_fault(
      HandleTraits<mln_render_session_object>::kind, session, HandleFault::Stale
    ));
    return nullptr;
  }
  return live;
}

auto enqueue_driver_operation(
  mln_render_session session, RenderDriverCallable work,
  const mln_completion* completion, ValuelessCompletion valueless
) -> mln_status {
  auto [deliver, make_work] = valueless_driver_work(std::move(work), valueless);
  return submit_driver_work(session, completion, std::move(deliver), make_work);
}

auto enqueue_driver_result_operation(
  mln_render_session session, RenderDriverResultCallable work,
  const mln_completion* completion, CompletionOperation::Delivery transfer
) -> mln_status {
  return submit_driver_work(
    session, completion, std::move(transfer),
    [work = std::move(work)](
      const std::shared_ptr<mln_render_session_object>& live,
      const std::shared_ptr<OperationObject>& operation
    ) {
      return RenderDriverWork{
        [live, operation, work]() {
          run_driver_result(live, operation, [&](std::any& result) {
            return work(*live, result);
          });
        },
        abandon_driver_result(operation)
      };
    }
  );
}

auto resume_driver_work(
  const std::weak_ptr<mln_render_session_object>& session
) noexcept -> void {
  const auto live = session.lock();
  if (live == nullptr) return;
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{live->control_mutex};
  publish_driver_work_locked(*live, wakes);
}

auto validate_render_session_attach_request(
  const mln_render_session_attach_options* options,
  const mln_render_session* out_session, const mln_completion* completion,
  std::shared_ptr<Completion>* checked
) -> mln_status {
  if (options == nullptr) {
    set_thread_error("render session attach options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_render_session_attach_options)) {
    set_thread_error("render session attach options size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_session == nullptr || *out_session != MLN_HANDLE_NULL) {
    set_thread_error("out_session must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto completion_status =
    make_completion(completion, checked_completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  if (checked) *checked = std::move(checked_completion);
  if (
    options->driver != MLN_RENDER_DRIVER_CORE_WORKER &&
    options->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ) {
    set_thread_error("render driver kind is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    options->producer_sync_kind != MLN_GPU_SYNC_CPU_COMPLETE &&
    options->producer_sync_kind != MLN_GPU_SYNC_OPEN_GL_FENCE
  ) {
    set_thread_error("producer synchronization kind is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto start_attach_render_session(
  std::shared_ptr<mln_render_session_object> session, RenderSessionKind kind,
  const mln_render_session_attach_options* options,
  mln_render_session_capabilities capabilities, mln_render_session* out_session,
  const mln_completion* completion, ValuelessCompletion valueless
) -> mln_status {
  if (session == nullptr) {
    set_thread_error("render session must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto request_status = validate_render_session_attach_request(
    options, out_session, completion, &checked_completion
  );
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }
  if (options->producer_sync_kind != session->producer_sync_kind) {
    set_thread_error("render target does not support this producer sync kind");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (capabilities.size < sizeof(mln_render_session_capabilities)) {
    set_thread_error("render session capabilities size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto map = handle_table<MapObject>().lease(session->map);
  if (map == nullptr) return recorded_handle_fault_status();
  if (map->runtime_state == nullptr) {
    set_thread_error("map has no live runtime");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto frame_wake_status = validate_wake(&options->frame_wake);
  if (frame_wake_status != MLN_STATUS_OK) return frame_wake_status;
  const auto driver_wake_status = validate_wake(&options->driver_work_wake);
  if (driver_wake_status != MLN_STATUS_OK) return driver_wake_status;
  const auto queue_lock_status = validate_queue_lock(&options->queue_lock);
  if (queue_lock_status != MLN_STATUS_OK) return queue_lock_status;
  if (options->queue_lock.lock != nullptr && !session->accepts_queue_lock) {
    set_thread_error("this render target does not take a queue lock");
    return MLN_STATUS_UNSUPPORTED;
  }
  auto frame_wake = std::make_shared<Wake>(options->frame_wake);
  auto driver_wake = std::make_shared<Wake>(options->driver_work_wake);
  auto queue_lock = options->queue_lock.lock == nullptr
                      ? std::shared_ptr<QueueLock>{}
                      : std::make_shared<QueueLock>(options->queue_lock);

  auto async = CompletionOperation{};
  const auto operation_status = create_completion_operation(
    std::move(checked_completion), valueless, async
  );
  if (operation_status != MLN_STATUS_OK) {
    return operation_status;
  }
  session->kind = kind;
  session->capabilities = capabilities;
  if (kind == RenderSessionKind::Texture) {
    // A borrowed ring's depth is the texture count its descriptor validated.
    const auto depth = session->texture.mode == TextureSessionMode::Owned
                         ? std::clamp(
                             capabilities.texture_ring_depth, 1u,
                             static_cast<uint32_t>(max_texture_ring_depth)
                           )
                         : capabilities.texture_ring_depth;
    session->capabilities.texture_ring_depth = depth;
    session->texture.slots.resize(depth);
  }
  session->capabilities.driver = options->driver;
  session->frame_wake = frame_wake;
  session->driver_wake = driver_wake;
  session->queue_lock = queue_lock;
  session->state = MLN_RENDER_SESSION_STATE_ATTACHING;
  const auto attach_status = map_attach_render_target_session(
    session->map, session.get(), session->map_object
  );
  if (attach_status != MLN_STATUS_OK) {
    return attach_status;
  }
  session->attached = true;
  // Every step from here can fail or throw after the map's session slot is
  // claimed. Unless the attachment commits, this returns the map, the
  // notices, and the handle, so the map never keeps a dangling session pointer.
  struct AttachUnwind {
    std::shared_ptr<mln_render_session_object> session;
    bool committed = false;
    ~AttachUnwind() {
      if (committed) return;
      {
        const auto lock = std::scoped_lock{session->control_mutex};
        session->stop_worker = true;
        session->worker_condition.notify_all();
      }
      reap_core_worker(*session);
      static_cast<void>(map_set_render_session_notices(session->map, {}));
      static_cast<void>(
        map_detach_render_target_session(session->map, session.get())
      );
      session->map_object = nullptr;
      if (session->self != MLN_HANDLE_NULL) {
        static_cast<void>(
          handle_table<mln_render_session_object>().remove(session->self)
        );
      }
    }
  } unwind{session};

  session->self = register_render_session(session);
  const auto weak_session = std::weak_ptr<mln_render_session_object>{session};
  const auto notices_status = map_set_render_session_notices(
    session->map,
    RenderSessionNotices{
      .update =
        [weak_session](uint64_t update_generation) noexcept {
          if (const auto live = weak_session.lock()) {
            notify_render_session_map_update(live.get(), update_generation);
          }
        },
      .still_image =
        [weak_session]() noexcept {
          if (const auto live = weak_session.lock()) {
            notify_render_session_still_image(live.get());
          }
        },
    }
  );
  if (notices_status != MLN_STATUS_OK) {
    return notices_status;
  }
  if (options->driver == MLN_RENDER_DRIVER_CORE_WORKER) {
    if (session->start_worker) {
      const auto worker_status =
        session->start_worker([session]() { run_core_worker(session); });
      if (worker_status != MLN_STATUS_OK) {
        return worker_status;
      }
    } else {
      session->worker =
        mln::core::WorkerThread{[session]() { run_core_worker(session); }};
    }
  }
  // Built before the commit below, because building it allocates and may
  // throw into the unwind.
  auto attach_work = RenderDriverWork{
    [session, attach_operation = async.operation]() {
      try {
        if (session->initialize_backend) {
          const auto initialize_status = session->initialize_backend(*session);
          if (initialize_status != MLN_STATUS_OK) {
            {
              const auto lock = std::scoped_lock{session->control_mutex};
              session->state = MLN_RENDER_SESSION_STATE_TARGET_LOST;
              ++session->generation;
            }
            attach_operation->complete(
              initialize_status, thread_last_error_message(), {}
            );
            return;
          }
        }
        if (
          auto* backend = renderer_backend(session.get()); backend != nullptr
        ) {
          const auto prime = mln::gfx::BackendScope{*backend};
        }
        {
          auto wakes = DeferredWakes{};
          const auto lock = std::scoped_lock{session->control_mutex};
          session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
          ++session->generation;
          arm_still_image_demand_locked(*session, wakes);
        }
        // Wake the driver whenever a worker thread posts a scheduler
        // task while the queue is idle, so queued results are delivered
        // even when no demand renders. Detach and abandon clear the
        // hook.
        session->scheduler.set_work_available_callback(
          [weak = std::weak_ptr<mln_render_session_object>{session}]() {
            auto live = weak.lock();
            if (live == nullptr) {
              return;
            }
            auto wakes = DeferredWakes{};
            const auto lock = std::scoped_lock{live->control_mutex};
            if (
              live->scheduler_service_queued ||
              (live->state != MLN_RENDER_SESSION_STATE_ATTACHED &&
               live->state != MLN_RENDER_SESSION_STATE_DETACHING)
            )
              return;
            live->scheduler_service_queued = true;
            const auto expected =
              live->demands.empty() ? nullptr : live->demands.front();
            push_driver_work_locked(
              *live,
              RenderDriverWork{
                .execute =
                  [live, expected]() {
                    service_scheduler_work(live, expected);
                  },
                .abandon = {},
                .ready = {},
                .renders_frame = expected != nullptr,
              },
              wakes
            );
          }
        );
        static_cast<void>(map_post_trigger_repaint(session->map));
        attach_operation->complete(MLN_STATUS_OK, {}, {});
      } catch (const std::exception& exception) {
        {
          const auto lock = std::scoped_lock{session->control_mutex};
          session->state = MLN_RENDER_SESSION_STATE_TARGET_LOST;
          ++session->generation;
        }
        attach_operation->complete(
          MLN_STATUS_NATIVE_ERROR, exception.what(), {}
        );
      }
    },
    [attach_operation = async.operation]() {
      attach_operation->complete(
        MLN_STATUS_TARGET_LOST, "render target was abandoned", {}
      );
    }
  };
  // Logged before the commit, so a throw while logging unwinds the attach.
  warn_on_scale_factor_mismatch(session->map, session->scale_factor);
  {
    // Nothing from here can fail, so the attachment commits under the queue
    // lock. The wakes and the completion are accepted before the driver can
    // take the work, so the driver, rather than this thread, delivers an
    // attach that finishes early, and queueing the work wakes a caller driver.
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{session->control_mutex};
    unwind.committed = true;
    *out_session = session->self;
    frame_wake->accept();
    driver_wake->accept();
    if (queue_lock != nullptr) queue_lock->accept();
    async.completion->accept();
    push_driver_work_locked(*session, std::move(attach_work), wakes);
  }
  // The map takes the target's width and height, ahead of any command that
  // the host submits once this returns.
  static_cast<void>(
    map_post_resize(session->map, session->width, session->height)
  );
  return MLN_STATUS_OK;
}

auto notify_render_session_map_update(
  mln_render_session_object* session, uint64_t update_generation
) noexcept -> void {
  if (session == nullptr) {
    return;
  }
  mln::testing::hit(mln::testing::SyncPoint::RenderSessionUpdateNoticed);
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{session->control_mutex};
  session->map_update_generation = update_generation;
  // A demand can render an update between the map storing it and this notice,
  // which then brings nothing the session has not drawn.
  if (update_generation != session->rendered_update_generation)
    session->pending_changes = true;
  // Queueing work publishes it to the driver. A demand parked by a full ring
  // keeps no work item, and an update frees no slot, so it needs no wake here.
  resume_update_waiting_demands_locked(*session, wakes);
}

auto notify_render_session_still_image(
  mln_render_session_object* session
) noexcept -> void {
  if (session == nullptr) return;
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{session->control_mutex};
  if (session->state != MLN_RENDER_SESSION_STATE_ATTACHED) return;
  if (map_still_image_generation(*session->map_object) != 0) {
    arm_still_image_demand_locked(*session, wakes);
    return;
  }
  // Another frame completed the image, or the map closed, so a still-image
  // demand that waits for an update would wait for nothing.
  auto& waiting = session->update_waiting_demands;
  for (auto entry = waiting.begin(); entry != waiting.end();) {
    if ((*entry)->token != MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE) {
      ++entry;
      continue;
    }
    finish_demand_locked(*session, **entry, MLN_RENDER_RESULT_NO_UPDATE, wakes);
    entry = waiting.erase(entry);
  }
}

auto validate_render_session(
  mln_render_session session, mln_render_session_object*& out_session
) -> mln_status {
  out_session = handle_table<mln_render_session_object>().resolve(session);
  return out_session == nullptr ? recorded_handle_fault_status()
                                : MLN_STATUS_OK;
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

auto unsupported_retarget(const char* message) -> mln_status {
  set_thread_error(message);
  return MLN_STATUS_UNSUPPORTED;
}

namespace {

// Whether a session of this shape can take a replacement target of that kind.
// Both retarget checks apply the same rule, one before the backend reads the
// descriptor and one on the driver thread.
auto retarget_kind_status(
  const mln_render_session_object& session, RetargetTargetKind kind
) -> mln_status {
  if (kind == RetargetTargetKind::Surface) {
    return session.kind == RenderSessionKind::Surface
             ? MLN_STATUS_OK
             : unsupported_retarget(
                 "session does not render through a native surface"
               );
  }
  if (session.kind != RenderSessionKind::Texture) {
    return unsupported_retarget(
      "session does not render into a caller-owned texture"
    );
  }
  return session.texture.mode == TextureSessionMode::Borrowed
           ? MLN_STATUS_OK
           : unsupported_retarget(
               "a session-owned texture is sized and "
               "replaced by its session; "
               "resize it instead"
             );
}

}  // namespace

auto validate_render_session_retarget_submission(
  mln_render_session session, RetargetTargetKind kind,
  const mln_completion* completion,
  std::shared_ptr<Completion>& checked_completion
) -> mln_status {
  const auto completion_status =
    make_completion(completion, checked_completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();

  const auto lock = std::scoped_lock{live->control_mutex};
  if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
    set_thread_error("render session is not attached");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->acquired_frame_count != 0) {
    set_thread_error(
      "cannot replace the render target while a texture frame is acquired"
    );
    return MLN_STATUS_INVALID_STATE;
  }
  return retarget_kind_status(*live, kind);
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
  {
    const auto lock = std::scoped_lock{out_session->control_mutex};
    if (out_session->acquired_frame_count != 0) {
      set_thread_error(
        "cannot replace the render target while a texture frame is acquired"
      );
      return MLN_STATUS_INVALID_STATE;
    }
  }
  return retarget_kind_status(*out_session, kind);
}

auto render_session_set_target(
  mln_render_session session, RetargetTargetKind kind,
  const mln_logical_extent& extent, uint32_t physical_width,
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
      reset_pushed_feature_state(*live);
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
    reset_pushed_feature_state(*live);
  }
  {
    // Snapshot readers see the extent and its generation together, as they do
    // across the sibling resize path.
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->control_mutex};
    live->rendered_update_generation = 0;
    live->rendered_target_generation = 0;
    live->pending_changes = true;
    live->width = extent.width;
    live->height = extent.height;
    live->physical_width = physical_width;
    live->physical_height = physical_height;
    live->scale_factor = extent.scale_factor;
    ++live->generation;
    // No frame is acquired, which the check above and the submission's hold
    // on acquisition ensure. Each slot starts over with its new texture, which
    // also lifts a quarantine: the texture that the host's GPU may still read
    // is no longer the slot's.
    if (kind == RetargetTargetKind::BorrowedTexture) {
      for (auto& slot : live->texture.slots) slot = RenderTextureSlot{};
    }
    // The new target holds no frame of the latest update, so a demand that
    // waits for an update has one to render now.
    resume_update_waiting_demands_locked(*live, wakes);
  }

  // The submission already sized the map to this extent.
  warn_on_scale_factor_mismatch(live->map, extent.scale_factor);
  return MLN_STATUS_OK;
}

namespace {

// Publishes both portions under the control lock so extent-changing
// submissions reach the map and driver in the same order.
auto submit_retarget(
  mln_render_session session, const mln_logical_extent& extent,
  RenderDriverCallable work, std::shared_ptr<Completion> completion,
  const ValuelessCompletion& valueless, RenderDriverAdmission admit
) -> mln_status {
  auto [deliver, make_work] = valueless_driver_work(std::move(work), valueless);
  admit.commit = [extent,
                  commit =
                    std::move(admit.commit)](mln_render_session_object& live) {
    if (commit) commit(live);
    static_cast<void>(map_post_resize(live.map, extent.width, extent.height));
  };
  return submit_driver_work(
    session, nullptr, std::move(deliver), make_work, admit,
    std::move(completion)
  );
}

}  // namespace

auto enqueue_borrowed_texture_retarget(
  mln_render_session session, std::size_t texture_count,
  const mln_logical_extent& extent, uint32_t physical_width,
  uint32_t physical_height, RenderTargetReplacer replace,
  std::shared_ptr<Completion> completion, ValuelessCompletion valueless
) -> mln_status {
  return submit_retarget(
    session, extent,
    [extent, physical_width, physical_height,
     replace = std::move(replace)](mln_render_session_object& target) {
      // Acquisition resumes once the replacement has run, whatever it
      // reported: the slots then hold no frame of the replaced textures.
      struct Settle {
        mln_render_session_object& session;
        explicit Settle(mln_render_session_object& live) : session(live) {}
        Settle(const Settle&) = delete;
        Settle(Settle&&) = delete;
        auto operator=(const Settle&) -> Settle& = delete;
        auto operator=(Settle&&) -> Settle& = delete;
        ~Settle() {
          const auto lock = std::scoped_lock{session.control_mutex};
          --session.texture.pending_retargets;
        }
      } settle{target};
      return render_session_set_target(
        target.self, RetargetTargetKind::BorrowedTexture, extent,
        physical_width, physical_height, replace
      );
    },
    completion, valueless,
    RenderDriverAdmission{
      .check =
        [texture_count](const mln_render_session_object& live) {
          if (live.acquired_frame_count != 0) {
            set_thread_error(
              "cannot replace the render target while a texture frame is "
              "acquired"
            );
            return MLN_STATUS_INVALID_STATE;
          }
          if (texture_count != live.texture.slots.size()) {
            set_thread_error(
              "texture_count must equal the ring depth the session attached "
              "with"
            );
            return MLN_STATUS_INVALID_ARGUMENT;
          }
          return MLN_STATUS_OK;
        },
      .commit =
        [](mln_render_session_object& live) {
          invalidate_unacquired_texture_frames_locked(live);
          ++live.texture.pending_retargets;
        },
    }
  );
}

auto enqueue_surface_retarget(
  mln_render_session session, const mln_logical_extent& extent,
  RenderTargetReplacer replace, std::shared_ptr<Completion> completion,
  ValuelessCompletion valueless
) -> mln_status {
  return submit_retarget(
    session, extent,
    [extent, replace = std::move(replace)](mln_render_session_object& target) {
      return render_session_set_target(
        target.self, RetargetTargetKind::Surface, extent,
        physical_dimension(extent.width, extent.scale_factor),
        physical_dimension(extent.height, extent.scale_factor), replace
      );
    },
    completion, valueless, {}
  );
}

namespace {

auto render_session_render_update_on_driver(
  mln_render_session_object& session, mln_render_result* out_result,
  bool* out_needs_repaint, uint64_t* out_map_update_generation,
  mln_map_view* out_view
) -> mln_status {
  auto* live = &session;
  if (!live->attached || !has_backend(live)) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }
  *out_result = MLN_RENDER_RESULT_NO_UPDATE;
  *out_needs_repaint = false;

  auto* backend = renderer_backend(live);
  if (backend == nullptr) {
    set_thread_error("render session renderer backend is not available");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mln::gfx::BackendScope{*backend};
  // Deliver tile and resource results first: destroying the tiles they retire
  // enqueues the GPU-release work that map_run_render_jobs() drains. This must
  // stay ahead of the early returns below, or a frame with no update to render
  // strands them.
  live->scheduler.drain();
  map_run_render_jobs(*live->map_object);

  auto [update, desired, update_generation] =
    map_latest_update_snapshot(*live->map_object);
  if (out_map_update_generation != nullptr) {
    *out_map_update_generation = update_generation;
  }
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
    update->transformState.getSize() != mln::Size{live->width, live->height}
  ) {
    *out_result = MLN_RENDER_RESULT_NO_UPDATE;
    return MLN_STATUS_OK;
  }

  if (live->kind == RenderSessionKind::Texture) {
    live->texture.backend->prepare_render_resources();
  }
  const auto wait_surface = [&]() -> std::optional<mln_status> {
    bool ready = true;
    if (
      const auto status = prepare_surface_frame(*live, ready);
      status != MLN_STATUS_OK
    ) {
      return status;
    }
    if (!ready) {
      *out_result = MLN_RENDER_RESULT_TARGET_NOT_READY;
      return MLN_STATUS_OK;
    }
    return std::nullopt;
  };
  if (const auto early = wait_surface()) {
    return *early;
  }
  if (live->renderer == nullptr) {
    try {
      live->renderer = std::make_unique<mln::Renderer>(
        *backend, static_cast<float>(live->scale_factor)
      );
#if defined(__ANDROID__) && defined(MLN_RENDER_BACKEND_OPENGL)
      // Surface and texture sessions share this construction. Android OpenGL
      // emulator hosts crash in the Goldfish translation layer when a later
      // symbol-layer update keeps retained drawables, so apply the same
      // mitigation MapLibre Android enables after it constructs a Renderer.
      if (android_opengl_emulator()) {
        live->renderer->enableAndroidEmulatorGoldfishMitigation(true);
      }
#endif
      live->frame_observer.set_delegate(
        map_renderer_observer(*live->map_object)
      );
      live->renderer->setObserver(&live->frame_observer);
    } catch (const std::exception& exception) {
      set_native_stage_error("creating renderer", exception);
      return MLN_STATUS_NATIVE_ERROR;
    }
  }

  // Lambdas capture this reference rather than the structured binding, which
  // OpenHarmony's clang does not accept as a capture.
  const auto& rendered_update = update;
  // Returns an early status, or nothing once the render attempt returns.
  const auto render_once = [&]() -> std::optional<mln_status> {
    try {
      live->frame_observer.begin_render();
      live->renderer->render(rendered_update);
#if defined(MLN_RENDER_BACKEND_VULKAN)
    } catch (const mln::vulkan::SurfaceNotReady&) {
      // The swapchain had no free image within the acquire bound. The frame
      // recorded nothing, so the host retries with this session.
      *out_result = MLN_RENDER_RESULT_TARGET_NOT_READY;
      return MLN_STATUS_OK;
#endif
    } catch (const RenderTargetNotReady&) {
      *out_result = MLN_RENDER_RESULT_TARGET_NOT_READY;
      return MLN_STATUS_OK;
    }
    return std::nullopt;
  };

  const auto sources_changed = live->rendered_sources != update->sources;
  const auto warmup =
    sources_changed &&
    feature_state_needs_warmup(*desired, live->rendered_source_ids, *update);
  if (warmup) {
    {
      const UnpresentedRender unpresented{live->frame_observer};
      if (const auto early = render_once()) {
        return *early;
      }
    }
    if (const auto early = wait_surface()) {
      return *early;
    }
  }
  if (sources_changed || live->pushed_feature_state.get() != desired.get()) {
    apply_feature_state_diff(
      *live->renderer, *update, *desired, live->applied_feature_state,
      live->rendered_source_ids
    );
    live->pushed_feature_state = std::move(desired);
  }
  if (sources_changed) {
    reconcile_feature_state_sources(
      live->rendered_source_ids, live->applied_feature_state, *update
    );
    live->rendered_sources = update->sources;
  }

  map_begin_render(*live->map_object, update_generation);
  if (const auto early = render_once()) {
    return *early;
  }
  // Absorb results that landed from worker threads during the render.
  live->scheduler.drain();
  // Static maps can return without drawing while style or tile data is pending.
  // Only a completed frame reports its view and replaces the backend state.
  if (!live->frame_observer.frame_completed()) {
    return MLN_STATUS_OK;
  }
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
  *out_view =
    map_view_from_transform(update->transformState, live->scale_factor);
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    live->rendered_target_generation = live->generation;
    live->rendered_update_generation = update_generation;
    live->pending_changes = false;
  }
  *out_result = MLN_RENDER_RESULT_RENDERED;
  *out_needs_repaint = live->frame_observer.needs_repaint();
  return MLN_STATUS_OK;
}

auto render_session_detach(mln_render_session_object& session) -> mln_status {
  auto* live = &session;
  if (!live->attached || !has_backend(live)) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }

  live->scheduler.set_work_available_callback({});
  // A quarantined slot's texture may still be in use by the host's GPU, and
  // nothing tells native when that use ends. A backend that owns such a
  // texture is released and never destroyed, as abandon does. A borrowed
  // ring's textures belong to the host, so its backend is destroyed.
  auto quarantine = false;
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    quarantine = live->texture.mode == TextureSessionMode::Owned &&
                 any_slot_quarantined_locked(*live);
  }
  TextureSessionBackend* quarantined_texture = nullptr;

  // Tear the renderer down before releasing the map's slot. The renderer holds
  // the map's forwarding observer, which the map's frontend owns; releasing the
  // slot first lets the runtime worker destroy the map and free that observer
  // while the drain and reset below still use it.
  {
    auto current = ScopedCurrentScheduler{live->scheduler};
    live->scheduler.drain();
    live->renderer.reset();
    reset_pushed_feature_state(*live);
    live->surface.backend.reset();
    if (quarantine)
      quarantined_texture = live->texture.backend.release();
    else
      live->texture.backend.reset();
    // Anything enqueued during teardown targets something already gone.
    live->scheduler.discard();
  }
  if (quarantined_texture != nullptr) {
    // Tile workers can still hold graphics objects of the released backend,
    // and the host may destroy its device once detach completes.
    map_quiesce_render_workers(live->map);
    quarantined_texture->quarantine();
  }

  const auto detach_status = map_detach_render_target_session(live->map, live);
  if (detach_status != MLN_STATUS_OK) {
    return detach_status;
  }
  live->attached = false;
  live->map_object = nullptr;
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    live->rendered_update_generation = 0;
    live->rendered_target_generation = 0;
    ++live->generation;
  }
  return MLN_STATUS_OK;
}

// CPU-side renderer maintenance, which every maintenance submission runs on
// the driver thread with the target's context current.
auto run_renderer_maintenance(
  mln_render_session_object& session, void (mln::Renderer::*action)()
) -> mln_status {
  if (!session.attached || !has_backend(&session)) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }
  mln::gfx::RendererBackend* backend = nullptr;
  if (
    const auto status = validate_renderer_backend(&session, backend);
    status != MLN_STATUS_OK
  ) {
    return status;
  }
  auto current = ScopedCurrentScheduler{session.scheduler};
  auto guard = mln::gfx::BackendScope{*backend};
  (session.renderer.get()->*action)();
  return MLN_STATUS_OK;
}

// Runs on the teardown lane, which retires every disposed session.
auto destroy_render_session(
  const std::shared_ptr<mln_render_session_object>& live
) -> mln_status {
  auto frame_wake = std::shared_ptr<Wake>{};
  auto driver_wake = std::shared_ptr<Wake>{};
  auto queue_lock = std::shared_ptr<QueueLock>{};
  {
    auto lock = std::unique_lock{live->control_mutex};
    if (
      live->views_invalidated &&
      live->state != MLN_RENDER_SESSION_STATE_DETACHED
    ) {
      if (
        live->active_views ||
        (!live->abandonment_complete &&
         live->abandonment_thread == current_owner_thread()) ||
        (live->driver_call_in_flight &&
         live->driver_call_thread == current_owner_thread())
      ) {
        set_thread_error(
          "render session still has active borrowed views or driver work"
        );
        return MLN_STATUS_BUSY;
      }
      live->worker_condition.wait(lock, [&] {
        return live->abandonment_complete;
      });
    }
    if (live->destruction_started) {
      return report_handle_fault(
        HandleTraits<mln_render_session_object>::kind, live->self,
        HandleFault::Stale
      );
    }
    if (
      live->state != MLN_RENDER_SESSION_STATE_DETACHED &&
      live->state != MLN_RENDER_SESSION_STATE_ABANDONED
    ) {
      set_thread_error(
        "render session must be detached or abandoned before it "
        "is destroyed"
      );
      return MLN_STATUS_INVALID_STATE;
    }
    if (
      live->state == MLN_RENDER_SESSION_STATE_DETACHED &&
      live->acquired_frame_count != 0
    ) {
      set_thread_error("cannot destroy while a texture frame is acquired");
      return MLN_STATUS_INVALID_STATE;
    }
    live->destruction_started = true;
    live->stop_worker = true;
    frame_wake = std::move(live->frame_wake);
    driver_wake = std::move(live->driver_wake);
    queue_lock = std::move(live->queue_lock);
    live->worker_condition.notify_all();
  }
  close_wakes(frame_wake, driver_wake);
  static_cast<void>(
    handle_table<mln_render_session_object>().remove(live->self)
  );
  reap_core_worker(*live);
  frame_wake.reset();
  driver_wake.reset();
  queue_lock.reset();
  return MLN_STATUS_OK;
}
}  // namespace

auto render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  std::shared_ptr<const RenderedFeatureList>& out_result
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
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
  auto line_string = mln::ScreenLineString{};
  auto clipped_box = std::optional<mln::ScreenBox>{};
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

  mln::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mln::gfx::BackendScope{*backend};
  auto features = std::vector<mln::Feature>{};
  switch (geometry->type) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      features = live->renderer->queryRenderedFeatures(
        mln::ScreenCoordinate{geometry->data.point.x, geometry->data.point.y},
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
  return create_rendered_feature_result(features, out_result);
}

auto render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  std::shared_ptr<const SourceFeatureList>& out_result
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
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
  mln::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto native_source_id = string_from_view(source_id);
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mln::gfx::BackendScope{*backend};
  const auto features =
    live->renderer->querySourceFeatures(native_source_id, *native_options);
  return create_source_feature_result(features, out_result);
}

auto validate_feature_extension_query(
  mln_buffer_view source_id, mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, mln_buffer_view arguments
) -> mln_status {
  if (
    !validate_non_empty_string(source_id, "source_id") ||
    !validate_non_empty_string(extension, "extension") ||
    !validate_non_empty_string(extension_field, "extension_field") ||
    !to_native_feature(feature) || !to_feature_extension_arguments(arguments)
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, mln_buffer_view arguments,
  std::string& out_result
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_render_session(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
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
  mln::gfx::RendererBackend* backend = nullptr;
  if (
    const auto backend_status = validate_renderer_backend(live, backend);
    backend_status != MLN_STATUS_OK
  ) {
    return backend_status;
  }

  auto query_feature = mln::Feature{std::move(*native_feature)};
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mln::gfx::BackendScope{*backend};
  const auto result = live->renderer->queryFeatureExtensions(
    string_from_view(source_id), query_feature, string_from_view(extension),
    string_from_view(extension_field), std::move(*native_arguments)
  );
  out_result = serialize_feature_extension_result(result);
  return MLN_STATUS_OK;
}

namespace {
auto publish_frame_result_locked(
  mln_render_session_object& session, mln_render_frame_result result,
  DeferredWakes& wakes
) noexcept -> void {
  // A batch view's stride is the record size, so every queued record carries
  // it whichever path built it.
  result.size = sizeof(mln_render_frame_result);
  if (result.disposition != MLN_RENDER_RESULT_RENDERED)
    result.generation = session.generation;
  if (
    result.token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE &&
    session.still_image_demands != 0
  )
    --session.still_image_demands;
  session.frame_results.push_back(result);
  if (session.frame_wake && !session.frame_wake_pending) {
    session.frame_wake_pending = true;
    wakes.frame(session.frame_wake);
  }
}

// Publishes `disposition` for a demand that ends without starting.
auto finish_demand_locked(
  mln_render_session_object& session, const mln_frame_demand& pending,
  mln_render_result disposition, DeferredWakes& wakes
) noexcept -> void {
  publish_frame_result_locked(
    session,
    mln_render_frame_result{
      sizeof(mln_render_frame_result),
      disposition,
      pending.token,
      session.map_update_generation,
      session.generation,
      0,
      false,
      {}
    },
    wakes
  );
}

auto finish_waiting_demands_locked(
  mln_render_session_object& session, mln_render_result disposition,
  DeferredWakes& wakes
) noexcept -> void {
  for (const auto& pending : session.update_waiting_demands)
    finish_demand_locked(session, *pending, disposition, wakes);
  session.update_waiting_demands.clear();
}

auto finish_demands_locked(
  mln_render_session_object& session, mln_render_result disposition,
  DeferredWakes& wakes
) noexcept -> void {
  finish_waiting_demands_locked(session, disposition, wakes);
  for (const auto& pending : session.demands)
    finish_demand_locked(session, *pending, disposition, wakes);
  session.demands.clear();
}

auto publish_frame_result(
  const std::shared_ptr<mln_render_session_object>& session,
  mln_render_frame_result result
) noexcept -> void {
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{session->control_mutex};
    publish_frame_result_locked(*session, result, wakes);
    if (
      result.token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE &&
      std::exchange(session->still_image_rearm, false)
    )
      arm_still_image_demand_locked(*session, wakes);
  }
  mln::testing::hit(mln::testing::SyncPoint::RenderFrameResultPublished);
}

auto fail_frame_target(
  const std::shared_ptr<mln_render_session_object>& session, mln_status status,
  std::exception_ptr reason = {}
) -> void;

// Delivers queued worker results to the renderer without rendering. Tile and
// placement continuations run on the session scheduler, and the renderer
// reports what they change to the map's run loop, which publishes the next
// update, so delivering them must not wait for a demand that happens to
// render.
auto deliver_pending_session_work(
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> mln_status {
  try {
    auto* backend = renderer_backend(session.get());
    if (backend == nullptr || session->map_object == nullptr)
      return MLN_STATUS_OK;
    auto current = ScopedCurrentScheduler{session->scheduler};
    auto guard = mln::gfx::BackendScope{*backend};
    session->scheduler.drain();
    map_run_render_jobs(*session->map_object);
    return MLN_STATUS_OK;
  } catch (const RenderTargetNotReady&) {
    fail_frame_target(session, MLN_STATUS_NOT_READY);
    return MLN_STATUS_NOT_READY;
#if defined(MLN_RENDER_BACKEND_VULKAN)
  } catch (const mln::vulkan::SurfaceNotReady&) {
    fail_frame_target(session, MLN_STATUS_NOT_READY);
    return MLN_STATUS_NOT_READY;
#endif
  } catch (...) {
    fail_frame_target(
      session, MLN_STATUS_TARGET_LOST, std::current_exception()
    );
    return MLN_STATUS_TARGET_LOST;
  }
}

// Driver work posted by the scheduler's repaint hook: drain the queued
// results, then let a pending demand re-evaluate. Without this, a session
// whose demands all resolve on the render-if-needed fast path never drains,
// stranding the tile results that a still image or the next update needs
// behind a demand that happens to render.
auto service_scheduler_work(
  const std::shared_ptr<mln_render_session_object>& session,
  std::shared_ptr<const mln_frame_demand> expected
) noexcept -> void {
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    session->scheduler_service_queued = false;
    if (
      session->state != MLN_RENDER_SESSION_STATE_ATTACHED &&
      session->state != MLN_RENDER_SESSION_STATE_DETACHING
    ) {
      return;
    }
  }
  deliver_pending_session_work(session);
  if (expected) run_frame_demand(session, expected);
}

// A demand with MLN_FRAME_DEMAND_WAIT_FOR_UPDATE found nothing to render in
// the map update of `evaluated` generation. It waits for the next update
// unless a barrier accepted after it ends the wait or no ring slot can take a
// frame. Returns whether the caller publishes `result`, which this sets to the
// terminal disposition when the demand finishes.
auto wait_for_map_update(
  const std::shared_ptr<mln_render_session_object>& session,
  const std::shared_ptr<const mln_frame_demand>& pending,
  std::uint64_t evaluated, mln_render_frame_result& result
) noexcept -> bool {
  mln::testing::hit(mln::testing::SyncPoint::RenderDemandWaits);
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{session->control_mutex};
  if (every_slot_quarantined_locked(*session)) {
    result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
    return true;
  }
  if (session->pending_frame_barriers != 0) {
    result.disposition = MLN_RENDER_RESULT_NO_UPDATE;
    return true;
  }
  // A still image's demand waits only while its image does, and the map's
  // notice ends a wait that outlasts the image.
  if (
    pending->token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE &&
    map_still_image_generation(*session->map_object) == 0
  )
    return true;
  // Waiting and queueing allocate. A demand that can do neither finishes as it
  // found the map.
  try {
    // Read from the map under the lock that notify takes after the map
    // stores an update, so an update this read misses finds the demand
    // waiting. The session's own generation can lag the map's, and comparing
    // with it would run the demand again until notify catches up.
    if (map_latest_update_generation(*session->map_object) == evaluated) {
      session->update_waiting_demands.push_back(pending);
      return false;
    }
    // An update landed after the demand looked, so the demand runs again
    // ahead of the demands accepted after it.
    session->demands.push_front(pending);
  } catch (...) {
    return true;
  }
  if (!queue_frame_demand_work_locked(*session, wakes)) {
    session->demands.pop_front();
    return true;
  }
  return false;
}

// Reports why the session lost its target as a map render error, where a host
// reads it. A failed status leaves its message in the driver thread's error,
// which no host call reads, so a report without an exception carries that.
auto report_target_lost(
  const mln_render_session_object& session, std::exception_ptr reason
) noexcept -> void {
  if (session.map_object == nullptr) return;
  auto* observer = map_renderer_observer(*session.map_object);
  if (observer == nullptr) return;
  try {
    if (reason == nullptr) {
      reason = std::make_exception_ptr(
        std::runtime_error{thread_last_error_message()}
      );
    }
    observer->onRenderError(reason);
  } catch (...) {
    // A report that cannot allocate is dropped; the session is lost anyway.
  }
}

auto fail_frame_target(
  const std::shared_ptr<mln_render_session_object>& session, mln_status status,
  std::exception_ptr reason
) -> void {
  if (status != MLN_STATUS_NOT_READY)
    report_target_lost(*session, std::move(reason));
  auto discarded = std::deque<RenderDriverWork>{};
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{session->control_mutex};
    session->pending_changes = true;
    session->rendered_update_generation = 0;
    session->rendered_target_generation = 0;
    if (status == MLN_STATUS_NOT_READY) return;
    session->state = MLN_RENDER_SESSION_STATE_TARGET_LOST;
    ++session->generation;
    invalidate_unacquired_texture_frames_locked(*session);
    finish_demands_locked(*session, MLN_RENDER_RESULT_TARGET_NOT_READY, wakes);
    discarded.swap(session->driver_work);
    session->stop_worker = true;
    session->worker_condition.notify_all();
  }
  for (auto& work : discarded) {
    if (work.abandon) work.abandon();
  }
}

auto run_frame_demand(
  const std::shared_ptr<mln_render_session_object>& session,
  const std::shared_ptr<const mln_frame_demand>& expected
) noexcept -> void {
  auto demand = std::shared_ptr<const mln_frame_demand>{};
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    // Work can retry a demand parked ahead of its own, but stops once its own
    // demand has ended, preserving the later demands' order with driver work.
    if (
      session->pending_frame_barriers != 0 ||
      std::find(session->demands.begin(), session->demands.end(), expected) ==
        session->demands.end() ||
      (session->state != MLN_RENDER_SESSION_STATE_ATTACHED &&
       session->state != MLN_RENDER_SESSION_STATE_DETACHING)
    )
      return;
    demand = session->demands.front();
    session->demands.pop_front();
    // Only what arrives while this attempt runs asks for another.
    if (demand->token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE)
      session->still_image_rearm = false;
  }
  auto selected_slot = std::optional<std::size_t>{};
  struct RenderingSlotGuard {
    std::shared_ptr<mln_render_session_object> session;
    std::optional<std::size_t>& slot;
    ~RenderingSlotGuard() {
      const auto lock = std::scoped_lock{session->control_mutex};
      if (slot) session->texture.slots[*slot].rendering = false;
    }
  } rendering_slot{session, selected_slot};
  auto result = mln_render_frame_result{
    .size = sizeof(mln_render_frame_result),
    .disposition = MLN_RENDER_RESULT_NO_UPDATE,
    .token = demand->token,
    .map_update_generation = map_latest_update_generation(*session->map_object),
    .generation = 0,
    .frame_generation = 0,
    .needs_repaint = false,
    .view = {},
  };
  const auto waits_for_update =
    (demand->flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE) != 0;
  if ((demand->flags & MLN_FRAME_DEMAND_FORCE) == 0) {
    auto unchanged = false;
    {
      const auto lock = std::scoped_lock{session->control_mutex};
      unchanged =
        !session->pending_changes &&
        session->rendered_target_generation == session->generation &&
        result.map_update_generation == session->rendered_update_generation;
    }
    if (unchanged) {
      // Nothing newer than the last rendered update exists, so honor the
      // render-if-needed contract without touching the target. A repaint
      // demand during an animation publishes a new update, which bumps the
      // generation and sets pending_changes before the next demand runs.
      // Rendering delivers queued worker results inside the render. This fast
      // path delivers them before publishing its result.
      const auto status = deliver_pending_session_work(session);
      result.disposition = MLN_RENDER_RESULT_NO_UPDATE;
      if (status != MLN_STATUS_OK) {
        result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
        publish_frame_result(session, result);
        return;
      }
      if (
        waits_for_update &&
        !wait_for_map_update(
          session, demand, result.map_update_generation, result
        )
      )
        return;
      publish_frame_result(session, result);
      return;
    }
  }
  auto ring_full = false;
  auto ring_quarantined = false;
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{session->control_mutex};
    auto& slots = session->texture.slots;
    ring_quarantined = every_slot_quarantined_locked(*session);
    if (ring_quarantined) {
      // No slot can take a frame again, so the demand ends here, behind the
      // driver work accepted before it. A demand that a full ring parked keeps
      // no work item, so the next one runs to reach its own turn.
      result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
      publish_frame_result_locked(*session, result, wakes);
      resume_parked_demand_locked(*session, wakes);
    } else if (!slots.empty()) {
      const auto busy = [](const RenderTextureSlot& value) {
        return value.acquired || value.rendering || value.quarantined;
      };
      const auto reusable =
        std::find_if(slots.begin(), slots.end(), [&](const auto& value) {
          return !busy(value) && !value.available;
        });
      const auto slot = reusable != slots.end()
                          ? reusable
                          : std::min_element(
                              slots.begin(), slots.end(),
                              [&](const auto& left, const auto& right) {
                                if (busy(left)) return false;
                                if (busy(right)) return true;
                                return left.result.frame_generation <
                                       right.result.frame_generation;
                              }
                            );
      ring_full = slot == slots.end() || busy(*slot);
      if (ring_full) {
        // Parking in the section that found the ring full keeps the demand
        // visible to a release or disposal on another thread, which queues
        // the work that resumes it.
        if (session->pending_frame_barriers != 0) {
          result.disposition = MLN_RENDER_RESULT_NO_UPDATE;
          publish_frame_result_locked(*session, result, wakes);
        } else {
          session->demands.push_front(demand);
        }
      } else {
        selected_slot = static_cast<std::size_t>(slot - slots.begin());
        slot->available = false;
        slot->rendering = true;
      }
    }
  }
  if (ring_quarantined) {
    deliver_pending_session_work(session);
    mln::testing::hit(mln::testing::SyncPoint::RenderFrameResultPublished);
    return;
  }
  if (ring_full) {
    deliver_pending_session_work(session);
    mln::testing::hit(mln::testing::SyncPoint::RenderFrameDemandParked);
    return;
  }
  auto rendered_map_generation = result.map_update_generation;
  try {
    if (selected_slot && session->texture.backend) {
      const auto status =
        session->texture.backend->select_render_slot(*selected_slot);
      if (status != MLN_STATUS_OK) {
        fail_frame_target(session, status);
        result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
        publish_frame_result(session, result);
        return;
      }
    }
    auto disposition = MLN_RENDER_RESULT_NO_UPDATE;
    auto needs_repaint = false;
    auto view = mln_map_view{};
    auto metadata = std::any{};
    auto producer_sync =
      mln_gpu_sync{sizeof(mln_gpu_sync), MLN_GPU_SYNC_CPU_COMPLETE, 0};
    // One backend scope spans the render and the frame's metadata, so the
    // scopes they open themselves nest in it and activate the backend once.
    auto scope = std::optional<mln::gfx::BackendScope>{};
    if (auto* backend = renderer_backend(session.get()); backend != nullptr)
      scope.emplace(*backend);
    {
      // A demand without MLN_FRAME_DEMAND_PRESENT still draws; the target keeps
      // whatever it presented last. Only a presenting target can honor this.
      const auto presents = (session->capabilities.flags &
                             MLN_RENDER_SESSION_CAPABILITY_PRESENTATION) != 0;
      const DiscardedPresent unpresented{
        presents && (demand->flags & MLN_FRAME_DEMAND_PRESENT) == 0
      };
      const auto status = render_session_render_update_on_driver(
        *session, &disposition, &needs_repaint, &rendered_map_generation, &view
      );
      if (status != MLN_STATUS_OK) {
        scope.reset();
        fail_frame_target(session, status);
        disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
      } else if (disposition == MLN_RENDER_RESULT_TARGET_NOT_READY) {
        scope.reset();
        fail_frame_target(session, MLN_STATUS_NOT_READY);
      }
    }
    result.disposition = disposition;
    result.map_update_generation = rendered_map_generation;
    if (disposition == MLN_RENDER_RESULT_RENDERED) {
      result.needs_repaint = needs_repaint;
      result.view = view;
      auto metadata_request = RenderFrameMetadata{};
      {
        const auto lock = std::scoped_lock{session->control_mutex};
        result.generation = session->generation;
        result.frame_generation = ++session->frame_generation;
        metadata_request = RenderFrameMetadata{
          .generation = session->generation,
          .frame_id = result.frame_generation,
          .slot = static_cast<uint32_t>(selected_slot.value_or(0)),
          .physical_width = session->physical_width,
          .physical_height = session->physical_height,
          .scale_factor = session->scale_factor,
        };
      }
      // Backend metadata is recorded here, on the driver thread, while the slot
      // is still the one that was rendered into. The acquiring host thread only
      // takes ownership of it without touching graphics state under the lock.
      const auto acquirable =
        selected_slot && session->texture.backend != nullptr &&
        (session->capabilities.flags &
         MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) != 0;
      if (acquirable) {
        const auto status = session->texture.backend->record_frame_metadata(
          metadata_request, metadata
        );
        if (status != MLN_STATUS_OK) {
          scope.reset();
          fail_frame_target(session, status);
          result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
          result.frame_generation = 0;
          result.needs_repaint = false;
          result.view = mln_map_view{};
          publish_frame_result(session, result);
          return;
        }
        producer_sync = session->texture.backend->producer_sync();
      }
      scope.reset();
      if (selected_slot) {
        {
          const auto lock = std::scoped_lock{session->control_mutex};
          auto& slot = session->texture.slots[*selected_slot];
          slot.result = result;
          slot.producer_sync = producer_sync;
          // A frame without its metadata cannot be acquired, so its slot stays
          // unavailable rather than hold back the newer frames behind it, and
          // the next demand renders into it first.
          slot.available = !acquirable || metadata.has_value();
          slot.backend_metadata = std::move(metadata);
          slot.rendering = false;
          if (
            session->texture.backend &&
            session->texture.backend->renders_ahead_of_gpu()
          )
            ++session->texture.gpu_frames_in_flight;
        }
      }
    }
    scope.reset();
  } catch (const RenderTargetNotReady&) {
    fail_frame_target(session, MLN_STATUS_NOT_READY);
    result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
#if defined(MLN_RENDER_BACKEND_VULKAN)
  } catch (const mln::vulkan::SurfaceNotReady&) {
    fail_frame_target(session, MLN_STATUS_NOT_READY);
    result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
#endif
  } catch (...) {
    fail_frame_target(
      session, MLN_STATUS_TARGET_LOST, std::current_exception()
    );
    result.disposition = MLN_RENDER_RESULT_TARGET_NOT_READY;
  }
  if (result.disposition != MLN_RENDER_RESULT_RENDERED) {
    result.frame_generation = 0;
    result.needs_repaint = false;
    result.view = mln_map_view{};
  }
  if (
    waits_for_update && result.disposition == MLN_RENDER_RESULT_NO_UPDATE &&
    !wait_for_map_update(session, demand, rendered_map_generation, result)
  )
    return;
  if (result.disposition == MLN_RENDER_RESULT_RENDERED)
    mln::testing::hit(mln::testing::SyncPoint::RenderFrameRendered);
  publish_frame_result(session, result);
}
}  // namespace

auto render_session_get_capabilities(
  mln_render_session session, mln_render_session_capabilities* out_capabilities
) -> mln_status {
  if (
    out_capabilities == nullptr ||
    out_capabilities->size < sizeof(*out_capabilities)
  ) {
    set_thread_error(
      "out_capabilities must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto lock = std::scoped_lock{live->control_mutex};
  *out_capabilities = live->capabilities;
  return MLN_STATUS_OK;
}

auto render_session_get_snapshot(
  mln_render_session session, mln_render_session_snapshot* out_snapshot
) -> mln_status {
  if (out_snapshot == nullptr || out_snapshot->size < sizeof(*out_snapshot)) {
    set_thread_error(
      "out_snapshot must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto lock = std::scoped_lock{live->control_mutex};
  *out_snapshot = mln_render_session_snapshot{
    .size = sizeof(*out_snapshot),
    .state = live->state,
    .extent = {live->width, live->height, live->scale_factor},
    .generation = live->generation,
    .map_update_generation = live->map_update_generation,
    .rendered_update_generation = live->rendered_update_generation,
    .frame_generation = live->frame_generation,
    .acquired_frame_count = live->acquired_frame_count,
    .pending_changes = live->pending_changes,
  };
  return MLN_STATUS_OK;
}

auto render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand
) -> mln_status {
  if (demand == nullptr || demand->size < sizeof(*demand)) {
    set_thread_error("demand must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_flags =
    static_cast<uint32_t>(MLN_FRAME_DEMAND_FORCE) |
    static_cast<uint32_t>(MLN_FRAME_DEMAND_PRESENT) |
    static_cast<uint32_t>(MLN_FRAME_DEMAND_WAIT_FOR_UPDATE);
  if ((demand->flags & ~known_flags) != 0) {
    set_thread_error("frame demand carries unknown policy flags");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (demand->flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE) != 0 &&
    (demand->flags & MLN_FRAME_DEMAND_FORCE) != 0
  ) {
    set_thread_error("a frame demand that waits for a map update cannot force");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (demand->token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE) {
    set_thread_error("the frame demand token is reserved for still images");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{live->control_mutex};
  if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
    set_thread_error("render session is not attached");
    return MLN_STATUS_INVALID_STATE;
  }
  if (every_slot_quarantined_locked(*live)) {
    set_thread_error("every texture slot of the render session is quarantined");
    return MLN_STATUS_INVALID_STATE;
  }
  const auto supersedes =
    [&](const std::shared_ptr<const mln_frame_demand>& pending) {
      return (demand->flags & MLN_FRAME_DEMAND_FORCE) == 0 &&
             pending->flags == demand->flags &&
             pending->token != MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE;
    };
  const auto supersede =
    [&](const std::shared_ptr<const mln_frame_demand>& pending) {
      publish_frame_result_locked(
        *live,
        mln_render_frame_result{
          sizeof(mln_render_frame_result),
          MLN_RENDER_RESULT_SUPERSEDED,
          pending->token,
          live->map_update_generation,
          live->generation,
          0,
          false,
          {}
        },
        wakes
      );
    };
  // Waiting demands can finish out of acceptance order.
  auto& waiting = live->update_waiting_demands;
  for (auto entry = waiting.begin(); entry != waiting.end();) {
    if (supersedes(*entry)) {
      supersede(*entry);
      entry = waiting.erase(entry);
    } else {
      ++entry;
    }
  }
  for (auto entry = live->demands.begin(); entry != live->demands.end();) {
    if (supersedes(*entry)) {
      supersede(*entry);
      entry = live->demands.erase(entry);
    } else {
      ++entry;
    }
  }
  const auto queued = std::make_shared<const mln_frame_demand>(*demand);
  live->demands.push_back(queued);
  // Superseded work must not start a later demand ahead of ordered driver work.
  push_driver_work_locked(
    *live,
    RenderDriverWork{
      .execute = [live, queued]() { run_frame_demand(live, queued); },
      .abandon = {},
      .ready = {},
      .renders_frame = true,
    },
    wakes
  );
  return MLN_STATUS_OK;
}

auto render_session_service_driver_work(mln_render_session session)
  -> mln_status {
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->disposal_requested.load()) {
      return report_handle_fault(
        HandleTraits<mln_render_session_object>::kind, session,
        HandleFault::Stale
      );
    }
    if (live->views_invalidated) {
      set_thread_error("render session has lost its target");
      return MLN_STATUS_TARGET_LOST;
    }
    if (live->capabilities.driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
      set_thread_error("render session is driven by its own core worker");
      return MLN_STATUS_INVALID_STATE;
    }
    if (
      live->graphics_thread &&
      *live->graphics_thread != mln::core::current_owner_thread()
    ) {
      set_thread_error(
        "render session driver work belongs to the thread that "
        "first serviced "
        "it"
      );
      return MLN_STATUS_WRONG_THREAD;
    }
    if (live->driver_call_in_flight) {
      set_thread_error("render session driver work is already in flight");
      return MLN_STATUS_BUSY;
    }
    if (!live->graphics_thread)
      live->graphics_thread = mln::core::current_owner_thread();
    live->driver_call_in_flight = true;
    live->driver_call_thread = current_owner_thread();
  }
  struct DriverCallGuard {
    std::shared_ptr<mln_render_session_object> session;
    ~DriverCallGuard() {
      mln::testing::hit(mln::testing::SyncPoint::RenderDriverExited);
      auto wakes = DeferredWakes{};
      auto* parked = static_cast<RetirementTask*>(nullptr);
      {
        const auto lock = std::scoped_lock{session->control_mutex};
        session->driver_call_in_flight = false;
        session->driver_call_thread.reset();
        parked = take_parked_retirements_locked(*session);
        session->worker_condition.notify_all();
        session->driver_wake_pending = driver_work_runnable_locked(*session);
        if (session->driver_wake_pending) wakes.driver(session->driver_wake);
      }
      resubmit_parked_retirements(parked);
    }
  } guard{live};
  auto wait_for_gpu = false;
  while (true) {
    auto item = RenderDriverWork{};
    {
      const auto lock = std::scoped_lock{live->control_mutex};
      if (!driver_work_runnable_locked(*live)) {
        // A frame that the frames in flight hold back asks once for their end.
        wait_for_gpu = frame_waits_for_gpu_locked(*live) &&
                       !std::exchange(live->texture.gpu_wait_pending, true);
        break;
      }
      item = std::move(live->driver_work.front());
      live->driver_work.pop_front();
    }
    // One throwing item must not strand the rest of the queue or skip its own
    // abandon report, exactly as the core worker treats it.
    try {
      auto execute = [&]() -> mln_status {
        if (item.execute) item.execute();
        return MLN_STATUS_OK;
      };
      static_cast<void>(mln::c_api::with_autorelease_pool(execute));
    } catch (...) {
      if (item.abandon) item.abandon();
    }
  }
  // Asked without the lock, since a backend may report at once.
  if (wait_for_gpu) live->texture.backend->when_gpu_done(GpuWorkDone{live});
  return MLN_STATUS_OK;
}

auto render_session_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch,
  mln_render_frame_batch_view* out_view
) -> mln_status {
  if (out_batch == nullptr || *out_batch != MLN_HANDLE_NULL) {
    set_thread_error("out_batch must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_view == nullptr || out_view->size < sizeof(*out_view)) {
    set_thread_error("out_view must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->frame_results.empty()) {
      live->frame_wake_pending = false;
      return MLN_STATUS_NOT_READY;
    }
  }
  // Allocating and registering the batch before the swap means a failure
  // loses no results: nothing after the swap can throw. The handle stays
  // private until this call returns it.
  const auto batch = std::make_shared<mln_render_frame_batch_object>();
  const auto handle =
    handle_table<mln_render_frame_batch_object>().insert(batch);
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    batch->results.swap(live->frame_results);
    live->frame_wake_pending = false;
  }
  // A concurrent drain can empty the queue between the two locks.
  if (batch->results.empty()) {
    render_frame_batch_release(handle);
    return MLN_STATUS_NOT_READY;
  }
  *out_batch = handle;
  *out_view = mln_render_frame_batch_view{
    .size = sizeof(mln_render_frame_batch_view),
    .result_size = sizeof(mln_render_frame_result),
    .results = batch->results.data(),
    .result_count = batch->results.size(),
  };
  return MLN_STATUS_OK;
}

auto render_frame_batch_release(mln_render_frame_batch batch) noexcept -> void {
  static_cast<void>(
    handle_table<mln_render_frame_batch_object>().remove(batch)
  );
}

auto render_session_acquire_frame(
  mln_render_session session, mln_acquired_frame* out_frame
) -> mln_status {
  if (out_frame == nullptr || *out_frame != MLN_HANDLE_NULL) {
    set_thread_error("out_frame must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  auto frame = std::shared_ptr<mln_acquired_frame_object>{};
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
    if (
      (live->capabilities.flags &
       MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) == 0
    ) {
      set_thread_error(
        "render session does not expose acquired texture frames"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
    // Every frame published before a queued replacement names a texture that
    // it replaces, which the host may destroy once the replacement completes.
    if (live->texture.pending_retargets != 0) return MLN_STATUS_NOT_READY;
    const auto slot = std::min_element(
      live->texture.slots.begin(), live->texture.slots.end(),
      [](const RenderTextureSlot& left, const RenderTextureSlot& right) {
        if (!left.available || left.acquired) return false;
        if (!right.available || right.acquired) return true;
        return left.result.frame_generation < right.result.frame_generation;
      }
    );
    if (slot == live->texture.slots.end() || !slot->available || slot->acquired)
      return MLN_STATUS_NOT_READY;
    // The driver recorded this when it published the frame; an empty record
    // means the backend had nothing to hand over yet.
    if (!slot->backend_metadata.has_value()) return MLN_STATUS_NOT_READY;
    frame = std::make_shared<mln_acquired_frame_object>();
    frame->session = live;
    frame->slot = static_cast<std::size_t>(slot - live->texture.slots.begin());
    frame->result = slot->result;
    frame->producer_sync = slot->producer_sync;
    frame->backend_metadata = std::move(slot->backend_metadata);
    slot->available = false;
    slot->acquired = true;
    ++live->acquired_frame_count;
  }
  try {
    *out_frame = handle_table<mln_acquired_frame_object>().insert(frame);
  } catch (...) {
    const auto lock = std::scoped_lock{live->control_mutex};
    auto& slot = live->texture.slots[frame->slot];
    slot.backend_metadata = std::move(frame->backend_metadata);
    slot.available = true;
    slot.acquired = false;
    --live->acquired_frame_count;
    throw;
  }
  return MLN_STATUS_OK;
}

auto lease_valid_acquired_frame(
  mln_acquired_frame frame,
  std::shared_ptr<mln_acquired_frame_object>& out_frame
) -> mln_status {
  auto live = handle_table<mln_acquired_frame_object>().lease(frame);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->valid.load()) {
    return report_handle_fault(
      HandleTraits<mln_acquired_frame_object>::kind, frame, HandleFault::Stale
    );
  }
  {
    const auto lock = std::scoped_lock{live->session->control_mutex};
    if (
      live->session->disposal_requested.load() ||
      live->session->views_invalidated ||
      live->session->state == MLN_RENDER_SESSION_STATE_ABANDONED ||
      live->session->state == MLN_RENDER_SESSION_STATE_TARGET_LOST
    ) {
      set_thread_error("render session no longer owns this frame's target");
      return MLN_STATUS_TARGET_LOST;
    }
  }
  out_frame = std::move(live);
  return MLN_STATUS_OK;
}

auto acquired_frame_view_begin(mln_acquired_frame frame, void** out_scope)
  -> mln_status {
  if (!out_scope) {
    set_thread_error("out_scope must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_scope = nullptr;
  auto live = handle_table<mln_acquired_frame_object>().lease(frame);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto lock = std::scoped_lock{live->session->control_mutex};
  if (!live->valid.load()) {
    return report_handle_fault(
      HandleTraits<mln_acquired_frame_object>::kind, frame, HandleFault::Stale
    );
  }
  if (
    live->session->views_invalidated ||
    live->session->disposal_requested.load() ||
    live->session->state == MLN_RENDER_SESSION_STATE_ABANDONED ||
    live->session->state == MLN_RENDER_SESSION_STATE_TARGET_LOST
  ) {
    set_thread_error("render session no longer owns this frame's target");
    return MLN_STATUS_TARGET_LOST;
  }
  if (live->active_views++ == 0) live->view_owner = live;
  ++live->session->active_views;
  *out_scope = live.get();
  return MLN_STATUS_OK;
}

void acquired_frame_view_end(void* scope) noexcept {
  if (!scope) return;
  auto& frame = *static_cast<mln_acquired_frame_object*>(scope);
  auto owner = std::shared_ptr<mln_acquired_frame_object>{};
  auto* parked = static_cast<RetirementTask*>(nullptr);
  {
    const auto lock = std::scoped_lock{frame.session->control_mutex};
    --frame.session->active_views;
    if (--frame.active_views == 0) owner = std::move(frame.view_owner);
    parked = take_parked_retirements_locked(*frame.session);
    frame.session->worker_condition.notify_all();
  }
  resubmit_parked_retirements(parked);
}

auto acquired_frame_get_result(
  mln_acquired_frame frame, mln_render_frame_result* out_result
) -> mln_status {
  if (out_result == nullptr || out_result->size < sizeof(*out_result)) {
    set_thread_error("out_result must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = std::shared_ptr<mln_acquired_frame_object>{};
  if (
    const auto status = lease_valid_acquired_frame(frame, live);
    status != MLN_STATUS_OK
  ) {
    return status;
  }
  *out_result = live->result;
  return MLN_STATUS_OK;
}

auto acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync
) -> mln_status {
  if (out_sync == nullptr || out_sync->size < sizeof(*out_sync)) {
    set_thread_error("out_sync must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = std::shared_ptr<mln_acquired_frame_object>{};
  if (
    const auto status = lease_valid_acquired_frame(frame, live);
    status != MLN_STATUS_OK
  ) {
    return status;
  }
  *out_sync = live->producer_sync;
  return MLN_STATUS_OK;
}

auto acquired_frame_release(
  mln_acquired_frame frame, const mln_gpu_sync* consumer_completion
) -> mln_status {
  if (frame == MLN_HANDLE_NULL) {
    set_thread_error("frame must be a live acquired frame handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto copied =
    consumer_completion != nullptr
      ? *consumer_completion
      : mln_gpu_sync{sizeof(mln_gpu_sync), MLN_GPU_SYNC_CPU_COMPLETE, 0};
  if (copied.size < sizeof(mln_gpu_sync)) {
    set_thread_error("mln_gpu_sync.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (copied.kind == MLN_GPU_SYNC_OPEN_GL_FENCE && copied.handle == 0) {
    set_thread_error("OpenGL fence must have a nonzero handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = handle_table<mln_acquired_frame_object>().lease(frame);
  if (live == nullptr) return recorded_handle_fault_status();
  auto wakes = DeferredWakes{};
  {
    const auto lock = std::scoped_lock{live->session->control_mutex};
    if (live->active_views) {
      set_thread_error("acquired frame has active borrowed views");
      return MLN_STATUS_BUSY;
    }
    if (!live->valid.load()) {
      set_thread_error("acquired frame was already released");
      return MLN_STATUS_INVALID_STATE;
    }
    // A valid frame keeps detach from destroying the backend. Reject an
    // unsupported sync before consuming the handle, so the host keeps it.
    if (
      live->session->texture.backend &&
      !live->session->texture.backend->supports_consumer_sync(copied.kind)
    ) {
      set_thread_error("render backend does not support this gpu sync kind");
      return MLN_STATUS_UNSUPPORTED;
    }
    auto claimed = true;
    if (!live->valid.compare_exchange_strong(claimed, false)) {
      set_thread_error("acquired frame was already released");
      return MLN_STATUS_INVALID_STATE;
    }
    if (copied.kind == MLN_GPU_SYNC_CPU_COMPLETE) {
      auto& session = *live->session;
      if (live->slot < session.texture.slots.size())
        session.texture.slots[live->slot].acquired = false;
      if (session.acquired_frame_count) --session.acquired_frame_count;
      resume_parked_demand_locked(session, wakes);
    }
  }
  const auto consumed = handle_table<mln_acquired_frame_object>().remove(frame);
  if (!consumed) {
    set_thread_error("acquired frame was already released");
    return MLN_STATUS_INVALID_STATE;
  }
  if (copied.kind == MLN_GPU_SYNC_CPU_COMPLETE) return MLN_STATUS_OK;
  const auto release = [consumed, copied]() {
    auto status = MLN_STATUS_OK;
    auto* backend = static_cast<TextureSessionBackend*>(nullptr);
    {
      const auto lock = std::scoped_lock{consumed->session->control_mutex};
      if (
        consumed->session->state == MLN_RENDER_SESSION_STATE_ABANDONED ||
        consumed->session->state == MLN_RENDER_SESSION_STATE_TARGET_LOST
      )
        status = MLN_STATUS_TARGET_LOST;
      else
        backend = consumed->session->texture.backend.get();
    }
    if (status == MLN_STATUS_OK && backend != nullptr)
      status = backend->release_consumer_sync(copied);
    {
      auto wakes = DeferredWakes{};
      const auto lock = std::scoped_lock{consumed->session->control_mutex};
      auto& slots = consumed->session->texture.slots;
      if (consumed->slot < slots.size()) {
        auto& slot = slots[consumed->slot];
        slot.acquired = false;
        // The slot is quarantined unless the host's GPU is done reading its
        // texture: after a failed sync wait, and after abandon when the host
        // handed over a GPU wait that native never ran. A release that races
        // abandon after it took the backend skips the backend's sync check,
        // so it can carry any kind, and abandon must keep the ring for it.
        if (
          (status != MLN_STATUS_OK && status != MLN_STATUS_TARGET_LOST) ||
          (status == MLN_STATUS_TARGET_LOST &&
           copied.kind != MLN_GPU_SYNC_CPU_COMPLETE)
        ) {
          quarantine_slot_locked(*consumed->session, slot, wakes);
        }
      }
      resume_parked_demand_locked(*consumed->session, wakes);
    }
    if (status != MLN_STATUS_OK && status != MLN_STATUS_TARGET_LOST) {
      mln::Log::Error(
        mln::Event::Render,
        "failed to retire an acquired frame; its texture slot is quarantined"
      );
    }
  };
  auto release_immediately = false;
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->session->control_mutex};
    if (live->session->acquired_frame_count)
      --live->session->acquired_frame_count;
    release_immediately =
      live->session->state == MLN_RENDER_SESSION_STATE_ABANDONED ||
      live->session->state == MLN_RENDER_SESSION_STATE_TARGET_LOST;
    if (!release_immediately) {
      push_driver_work_locked(
        *live->session, RenderDriverWork{release, release}, wakes
      );
    }
  }
  if (release_immediately) release();
  return MLN_STATUS_OK;
}

namespace {
struct DriverMapCompletion {
  std::mutex mutex;
  std::shared_ptr<OperationObject> operation;
  unsigned remaining = 2;
  mln_status status = MLN_STATUS_OK;
  std::string diagnostic;
  CommandGenerations generations;

  // The map's half passes the generations that its resize command reported.
  auto complete(
    mln_status next_status, std::string next_diagnostic,
    std::optional<CommandGenerations> map_generations = std::nullopt
  ) -> void {
    {
      const auto lock = std::scoped_lock{mutex};
      if (map_generations) generations = *map_generations;
      if (next_status != MLN_STATUS_OK || status == MLN_STATUS_OK) {
        status = next_status;
        diagnostic = std::move(next_diagnostic);
      }
      if (--remaining != 0) return;
    }
    operation->complete(status, std::move(diagnostic), generations);
  }
};

auto make_ordered_resize_work(
  const std::shared_ptr<mln_render_session_object>& session,
  const std::shared_ptr<DriverMapCompletion>& completion,
  mln_logical_extent extent
) -> RenderDriverWork {
  return RenderDriverWork{
    [session, completion, extent]() {
      auto attached = false;
      {
        const auto lock = std::scoped_lock{session->control_mutex};
        attached = session->state == MLN_RENDER_SESSION_STATE_ATTACHED;
        if (attached) invalidate_unacquired_texture_frames_locked(*session);
      }
      if (!attached) {
        completion->complete(
          MLN_STATUS_INVALID_STATE,
          "render session detached before the resize applied"
        );
        return;
      }
      try {
        const auto width =
          physical_dimension(extent.width, extent.scale_factor);
        const auto height =
          physical_dimension(extent.height, extent.scale_factor);
        if (session->kind == RenderSessionKind::Surface)
          session->surface.backend->resize(width, height);
        else
          session->texture.backend->resize({width, height});
        {
          auto wakes = DeferredWakes{};
          const auto lock = std::scoped_lock{session->control_mutex};
          session->width = extent.width;
          session->height = extent.height;
          session->physical_width = width;
          session->physical_height = height;
          ++session->generation;
          session->pending_changes = true;
          session->rendered_update_generation = 0;
          session->rendered_target_generation = 0;
          resume_update_waiting_demands_locked(*session, wakes);
        }
        completion->complete(MLN_STATUS_OK, {});
      } catch (const std::exception& exception) {
        completion->complete(MLN_STATUS_NATIVE_ERROR, exception.what());
      } catch (...) {
        completion->complete(
          MLN_STATUS_NATIVE_ERROR, "render target resize failed"
        );
      }
    },
    [completion]() {
      completion->complete(MLN_STATUS_TARGET_LOST, "target abandoned");
    }
  };
}
}  // namespace

auto render_session_resize_start(
  mln_render_session session, mln_logical_extent extent,
  const mln_completion* completion
) -> mln_status {
  const auto valid = validate_logical_extent(
    extent, "render target dimensions and scale factor must be positive"
  );
  if (valid != MLN_STATUS_OK) return valid;
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  mln::testing::hit(mln::testing::SyncPoint::RenderExtentSubmissionStarted);
  auto runtime = std::shared_ptr<RuntimeObject>{};
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
    if (
      live->kind == RenderSessionKind::Texture &&
      live->texture.mode == TextureSessionMode::Borrowed
    ) {
      set_thread_error(
        "a caller-owned texture is sized by its owner; hand over a "
        "replacement "
        "with the borrowed-texture set_target function for this backend"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
    if (live->acquired_frame_count != 0) {
      set_thread_error("cannot resize while a texture frame is acquired");
      return MLN_STATUS_INVALID_STATE;
    }
    // The renderer bakes its pixel ratio into compiled shaders, so only an
    // attachment or a target replacement can change the session's scale.
    if (extent.scale_factor != live->scale_factor) {
      set_thread_error(
        "render session scale_factor is set at attachment or by a target "
        "replacement; a resize cannot change it"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    runtime = live->map_object->runtime_state;
  }
  auto async = CompletionOperation{};
  const auto registered = create_completion_operation(
    completion,
    [](
      const std::shared_ptr<Completion>& state, mln_status status,
      std::string diagnostic, std::any value
    ) {
      // Empty when the resize failed before the map ran it.
      const auto* generations = std::any_cast<CommandGenerations>(&value);
      complete_command(
        state,
        status != MLN_STATUS_OK ? MLN_COMMAND_DISPOSITION_FAILED
                                : MLN_COMMAND_DISPOSITION_COMMITTED,
        status, generations != nullptr ? *generations : CommandGenerations{},
        std::move(diagnostic)
      );
    },
    async, true
  );
  if (registered != MLN_STATUS_OK) return registered;
  const auto fail_on_worker = [&](mln_status status, std::string diagnostic) {
    return submit_runtime_command(
      runtime,
      [operation = async.operation, status,
       diagnostic = std::move(diagnostic)]() mutable {
        operation->complete(status, std::move(diagnostic), {});
      },
      async.completion
    );
  };
  auto joined = std::make_shared<DriverMapCompletion>();
  joined->operation = async.operation;
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    live->pending_changes = true;
    // An owned-texture ring may contain several completed frames. None of its
    // old-size, unacquired entries may survive an accepted resize and outrank
    // the first new-size frame when the host next acquires the oldest result.
    invalidate_unacquired_texture_frames_locked(*live);
  }
  auto context = std::make_unique<std::shared_ptr<DriverMapCompletion>>(joined);
  const auto map_completion = mln_completion{
    .size = sizeof(mln_completion),
    .callback =
      [](void* data, const mln_completion_result* result) {
        const auto& joined =
          *static_cast<std::shared_ptr<DriverMapCompletion>*>(data);
        joined->complete(
          result->status, string_from_view(result->diagnostic),
          CommandGenerations{
            .snapshot = result->generation,
            .render_update = result->render_update_generation,
          }
        );
      },
    .user_data = context.get(),
    .release_user_data =
      [](void* data) {
        delete static_cast<std::shared_ptr<DriverMapCompletion>*>(data);
      },
    .queue = MLN_HANDLE_NULL,
  };
  // Publish the map and driver work together. A concurrent extent change
  // must follow this submission on both queues.
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state == MLN_RENDER_SESSION_STATE_ATTACHED) {
      auto work = make_ordered_resize_work(live, joined, extent);
      const auto post = map_post_resize(
        live->map, extent.width, extent.height, &map_completion
      );
      if (post != MLN_STATUS_OK)
        return fail_on_worker(post, thread_last_error_message());
      static_cast<void>(context.release());
      mln::testing::hit(mln::testing::SyncPoint::RenderExtentPublished);
      if (async.completion) async.completion->accept();
      push_driver_work_locked(*live, std::move(work), wakes);
      return MLN_STATUS_OK;
    }
  }
  return fail_on_worker(
    MLN_STATUS_INVALID_STATE,
    "render session detached before the resize applied"
  );
}

auto render_session_barrier_start(
  mln_render_session session, const mln_completion* completion
) -> mln_status {
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto async = CompletionOperation{};
  const auto status = create_completion_operation(
    completion, valueless_completion<&mln_render_session_barrier>(), async
  );
  if (status != MLN_STATUS_OK) return status;
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
    ++live->pending_frame_barriers;
    finish_demands_locked(*live, MLN_RENDER_RESULT_NO_UPDATE, wakes);
    auto work = RenderDriverWork{
      [live, operation = async.operation]() {
        {
          auto wakes = DeferredWakes{};
          const auto lock = std::scoped_lock{live->control_mutex};
          --live->pending_frame_barriers;
          arm_still_image_demand_locked(*live, wakes);
        }
        operation->complete(MLN_STATUS_OK, {}, {});
      },
      [operation = async.operation]() {
        operation->complete(MLN_STATUS_TARGET_LOST, "target abandoned", {});
      }
    };
    async.completion->accept();
    push_driver_work_locked(*live, std::move(work), wakes);
  }
  return MLN_STATUS_OK;
}

namespace {

auto maintenance_completion(RenderSessionMaintenance maintenance)
  -> ValuelessCompletion {
  switch (maintenance) {
    case RenderSessionMaintenance::ReduceMemoryUse:
      return valueless_completion<&mln_render_session_reduce_memory_use>();
    case RenderSessionMaintenance::ClearData:
      return valueless_completion<&mln_render_session_clear_data>();
    case RenderSessionMaintenance::DumpDebugLogs:
      return valueless_completion<&mln_render_session_dump_debug_logs>();
  }
  // An invalid kind fails in the driver work, so this never reports success.
  return valueless_completion<&mln_render_session_reduce_memory_use>();
}

}  // namespace

auto render_session_maintenance_start(
  mln_render_session session, RenderSessionMaintenance maintenance,
  const mln_completion* completion
) -> mln_status {
  return enqueue_driver_operation(
    session,
    [maintenance](mln_render_session_object& live) {
      switch (maintenance) {
        case RenderSessionMaintenance::ReduceMemoryUse:
          return run_renderer_maintenance(
            live, &mln::Renderer::reduceMemoryUse
          );
        case RenderSessionMaintenance::ClearData: {
          const auto status =
            run_renderer_maintenance(live, &mln::Renderer::clearData);
          if (status != MLN_STATUS_OK) return status;
          reset_pushed_feature_state(live);
          return map_post_trigger_repaint(live.map);
        }
        case RenderSessionMaintenance::DumpDebugLogs:
          return run_renderer_maintenance(live, &mln::Renderer::dumpDebugLogs);
      }
      set_thread_error("render session maintenance kind is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    },
    completion, maintenance_completion(maintenance)
  );
}

namespace {
// Runs on the driver. Detaches the target and ends outstanding demands.
// Disposal drops their results because the host no longer drains them.
auto finish_detach(const std::shared_ptr<mln_render_session_object>& live)
  -> mln_status {
  static_cast<void>(map_set_render_session_notices(live->map, {}));
  const auto detach_status = render_session_detach(*live);
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->control_mutex};
    live->state = detach_status == MLN_STATUS_OK
                    ? MLN_RENDER_SESSION_STATE_DETACHED
                    : MLN_RENDER_SESSION_STATE_TARGET_LOST;
    ++live->generation;
    // A demand parked by a full texture ring, or waiting for a map update,
    // keeps no work item, so detach is the last place that can give it its
    // terminal result.
    if (live->disposal_requested.load()) {
      live->update_waiting_demands.clear();
      live->demands.clear();
    } else {
      finish_demands_locked(*live, MLN_RENDER_RESULT_TARGET_NOT_READY, wakes);
    }
  }
  return detach_status;
}
}  // namespace

auto render_session_detach_start(
  mln_render_session session, const mln_completion* completion
) -> mln_status {
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->state != MLN_RENDER_SESSION_STATE_ATTACHED) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
    if (live->acquired_frame_count != 0) {
      set_thread_error("cannot detach while a texture frame is acquired");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto async = CompletionOperation{};
  const auto status = create_completion_operation(
    completion, valueless_completion<&mln_render_session_detach>(), async
  );
  if (status != MLN_STATUS_OK) return status;
  // Built before the lock below, because building it allocates and may throw.
  auto detach_work = RenderDriverWork{
    [live, operation = async.operation]() {
      const auto detach_status = finish_detach(live);
      operation->complete(detach_status, {}, {});
    },
    [operation = async.operation]() {
      operation->complete(MLN_STATUS_TARGET_LOST, "target abandoned", {});
    }
  };
  {
    auto wakes = DeferredWakes{};
    const auto lock = std::scoped_lock{live->control_mutex};
    if (
      live->state != MLN_RENDER_SESSION_STATE_ATTACHED ||
      live->acquired_frame_count != 0
    ) {
      set_thread_error("render session is not attached");
      return MLN_STATUS_INVALID_STATE;
    }
    live->state = MLN_RENDER_SESSION_STATE_DETACHING;
    ++live->generation;
    // The work is queued under the lock that marks the session detaching. An
    // abandon that takes the lock later finds the work in the queue and
    // completes it as abandoned; one queued after the abandon emptied the
    // queue would never run, and the detach would never complete. The
    // completion is accepted first, so the driver, rather than this thread,
    // delivers it.
    async.completion->accept();
    push_driver_work_locked(*live, std::move(detach_work), wakes);
  }
  mln::testing::hit(mln::testing::SyncPoint::RenderDetachQueued);
  return MLN_STATUS_OK;
}

auto render_session_abandon(
  mln_render_session session, mln_render_abandon_result* out_result
) -> mln_status {
  if (out_result == nullptr || out_result->size < sizeof(*out_result)) {
    set_thread_error("out_result must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  return abandon_render_session(live, out_result);
}

auto render_session_abandon_keeping_graphics(
  mln_render_session session, mln_render_abandon_result* out_result
) -> mln_status {
  if (out_result == nullptr || out_result->size < sizeof(*out_result)) {
    set_thread_error("out_result must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  return abandon_render_session(live, out_result, AbandonedGraphics::Keep);
}

namespace {
// The graphics objects that abandon takes from a session, which nothing else
// can reach once it has taken them. Whatever retirement leaves behind, such as
// when an exception skips it, is kept rather than destroyed unprepared.
struct AbandonedObjects {
  AbandonedObjects() = default;
  AbandonedObjects(const AbandonedObjects&) = delete;
  AbandonedObjects(AbandonedObjects&&) = delete;
  auto operator=(const AbandonedObjects&) -> AbandonedObjects& = delete;
  auto operator=(AbandonedObjects&&) -> AbandonedObjects& = delete;
  ~AbandonedObjects() { static_cast<void>(keep()); }

  auto keep() noexcept -> uint32_t {
    auto kept = uint32_t{0};
    if (renderer.release() != nullptr) ++kept;
    if (auto* backend = surface.release(); backend != nullptr) {
      backend->quarantine();
      ++kept;
    }
    if (auto* backend = texture.release(); backend != nullptr) {
      backend->quarantine();
      ++kept;
    }
    return kept;
  }

  std::unique_ptr<mln::Renderer> renderer;
  std::unique_ptr<SurfaceSessionBackend> surface;
  std::unique_ptr<TextureSessionBackend> texture;
};

// Destroys what abandon took from a session when that is safe on this thread,
// keeps the rest until the process exits, and returns how many it kept.
// `keep_ring` says the host's GPU may still read a texture of the session's
// ring, which lives in the texture backend. The renderer goes either way, as
// it does when detach keeps a ring: the host's GPU never reads its objects.
auto retire_abandoned_objects(
  mln_render_session_object& session, AbandonedObjects& objects, bool keep_ring
) -> uint32_t {
  const auto teardown_allowed = [&] {
    if (objects.surface != nullptr)
      return objects.surface->allows_off_thread_teardown();
    if (objects.texture != nullptr)
      return objects.texture->allows_off_thread_teardown();
    return true;
  };
  // Once exit begins, the host's runtime and its graphics driver may already
  // be gone.
  if (process_exiting() || !teardown_allowed()) return objects.keep();
  const auto prepared = (objects.surface == nullptr ||
                         objects.surface->prepare_off_thread_teardown()) &&
                        (objects.texture == nullptr ||
                         objects.texture->prepare_off_thread_teardown());
  // A Vulkan drain that exit refused waited for nothing.
  if (!prepared || process_exiting()) return objects.keep();
  // The driver thread created these objects, and this thread destroys them.
  // Vulkan and Metal objects have no thread affinity. mbgl's vulkan::Context
  // checks its creating thread with MBGL_VERIFY_THREAD, which only builds
  // with assertions enabled; every preset is a Release build, so a debug
  // preset would need this teardown on the driver thread instead.
  auto destroy = [&]() -> mln_status {
    // Destruction can schedule mailbox work, which nothing runs afterwards.
    const auto current = ScopedCurrentScheduler{session.scheduler};
    // The renderer draws through the backend, so it goes first.
    objects.renderer.reset();
    objects.surface.reset();
    if (!keep_ring) objects.texture.reset();
    session.scheduler.discard();
    return MLN_STATUS_OK;
  };
  static_cast<void>(mln::c_api::with_autorelease_pool(destroy));
  return objects.keep();
}

auto abandon_render_session(
  const std::shared_ptr<mln_render_session_object>& live,
  mln_render_abandon_result* out_result, AbandonedGraphics graphics
) -> mln_status {
  auto discarded = std::deque<RenderDriverWork>{};
  auto frame_wake = std::shared_ptr<Wake>{};
  auto driver_wake = std::shared_ptr<Wake>{};
  auto queue_lock = std::shared_ptr<QueueLock>{};
  auto objects = AbandonedObjects{};
  auto keep_ring = false;
  {
    // Declared before the lock, so the wakes owed by the results published
    // below run after it is released and before the wakes are dropped.
    auto wakes = DeferredWakes{};
    auto lock = std::unique_lock{live->control_mutex};
    // A core worker's call can still be running after it published the frame
    // result or completion the host acted on, and nothing public observes its
    // end, so abandon waits it out. The worker takes no further call while an
    // abandon waits. A caller driver's call belongs to the host, which sees it
    // return, so abandon during one is busy, as is abandon from inside the
    // session's own driver call.
    auto waiting_for_call = false;
    const auto stop_waiting = [&]() noexcept {
      if (!waiting_for_call) return;
      waiting_for_call = false;
      --live->abandon_waiters;
      live->worker_condition.notify_all();
    };
    while (true) {
      if (live->active_views) {
        stop_waiting();
        set_thread_error("render session has active borrowed views");
        return MLN_STATUS_BUSY;
      }
      if (
        live->state == MLN_RENDER_SESSION_STATE_DETACHED ||
        live->state == MLN_RENDER_SESSION_STATE_ABANDONED
      ) {
        stop_waiting();
        set_thread_error("render session has already released its target");
        return MLN_STATUS_INVALID_STATE;
      }
      if (!live->driver_call_in_flight) break;
      if (
        live->capabilities.driver != MLN_RENDER_DRIVER_CORE_WORKER ||
        live->driver_call_thread == current_owner_thread()
      ) {
        stop_waiting();
        set_thread_error("render session driver work is already in flight");
        return MLN_STATUS_BUSY;
      }
      if (!waiting_for_call) {
        waiting_for_call = true;
        ++live->abandon_waiters;
        lock.unlock();
        mln::testing::hit(mln::testing::SyncPoint::RenderAbandonWaits);
        lock.lock();
      }
      live->worker_condition.wait(lock, [&] {
        return !live->driver_call_in_flight;
      });
    }
    stop_waiting();
    live->views_invalidated = true;
    live->abandonment_thread = current_owner_thread();
    live->state = MLN_RENDER_SESSION_STATE_ABANDONED;
    live->attached = false;
    live->stop_worker = true;
    discarded.swap(live->driver_work);
    if (live->disposal_requested.load()) {
      live->update_waiting_demands.clear();
      live->demands.clear();
    } else {
      finish_demands_locked(*live, MLN_RENDER_RESULT_TARGET_NOT_READY, wakes);
    }
    // Another thread, such as a request_frame that superseded a demand, can
    // have set the frame wake pending and released the lock without invoking
    // the wake yet. Closing the wake below makes that late notify do nothing,
    // so abandon invokes the frame wake for any queued result itself, whatever
    // the coalescing flag says.
    if (!live->frame_results.empty() && live->frame_wake != nullptr) {
      live->frame_wake_pending = true;
      wakes.frame(live->frame_wake);
    }
    // The publish and release paths read the wakes and the graphics objects
    // under this lock, so both move out here. The graphics objects are
    // retired, and the wakes dropped, after the lock: destruction makes
    // graphics calls, and releasing host user data can run arbitrary host
    // code.
    objects.renderer = std::move(live->renderer);
    objects.surface = std::move(live->surface.backend);
    objects.texture = std::move(live->texture.backend);
    frame_wake = std::move(live->frame_wake);
    driver_wake = std::move(live->driver_wake);
    queue_lock = std::move(live->queue_lock);
    ++live->generation;
    live->worker_condition.notify_all();
  }
  close_wakes(frame_wake, driver_wake);
  static_cast<void>(map_set_render_session_notices(live->map, {}));
  for (auto& item : discarded) {
    if (item.abandon) item.abandon();
  }
  // The host's GPU may still read the texture of a session-owned slot that a
  // frame holds or that a disposed frame quarantined, and nothing tells native
  // when that read ends, so such a ring is kept. This reads the slots after
  // the loop above, which ran the releases still queued for the driver. A
  // release with CPU_COMPLETE, by which the host attests that its GPU read is
  // done, lets go of its slot. A release with a GPU wait quarantines its slot
  // instead, because abandon never runs that wait: one can arrive between
  // taking the backend and this read, when no backend checks its kind.
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    keep_ring = live->texture.mode == TextureSessionMode::Owned &&
                std::ranges::any_of(live->texture.slots, [](const auto& slot) {
                  return slot.acquired || slot.quarantined;
                });
  }
  live->scheduler.set_work_available_callback({});
  live->scheduler.discard();
  // Tile workers can still hold the renderer's atlas, which holds the host's
  // graphics device. Drain them before the renderer is retired, so that no
  // library thread touches the host's graphics objects once abandon returns.
  map_quiesce_render_workers(live->map);
  // Retired before the map's slot is released, because the renderer holds
  // the map's forwarding observer, as detach explains.
  const auto kept = graphics == AbandonedGraphics::Keep
                      ? objects.keep()
                      : retire_abandoned_objects(*live, objects, keep_ring);
  static_cast<void>(map_detach_render_target_session(live->map, live.get()));
  live->map_object = nullptr;
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    live->abandonment_complete = true;
    live->abandonment_thread.reset();
    live->worker_condition.notify_all();
  }
  frame_wake.reset();
  driver_wake.reset();
  // Retirement took the lock out of the backend's queue registration, by
  // destroying or keeping the backend, so this drops the last reference and
  // runs its release.
  queue_lock.reset();
  *out_result = mln_render_abandon_result{
    sizeof(*out_result),
    kept == 0 ? MLN_RENDER_ABANDON_DISPOSITION_CLEAN
              : MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED,
    kept, 0
  };
  return MLN_STATUS_OK;
}

auto abandon_driver_work(std::deque<RenderDriverWork>& queue) noexcept -> void {
  for (auto& item : queue) {
    if (item.abandon) item.abandon();
  }
  queue.clear();
}

// Runs on a disposed session's core worker, which exits afterwards. The host
// keeps its graphics objects alive until the session's wakes are released,
// which happens only after this returns, so the worker can still detach and
// free the renderer and backends. Anything that keeps the detach from
// finishing falls back to the teardown lane's abandon, which retires whatever
// the session still holds.
auto detach_disposed_session(
  const std::shared_ptr<mln_render_session_object>& session
) noexcept -> void {
  auto discarded = std::deque<RenderDriverWork>{};
  auto detaching = false;
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    session->disposal_detach = false;
    // An acquire that leased the session before disposal can still have taken
    // a frame, which keeps the target in the host's hands.
    detaching = session->state == MLN_RENDER_SESSION_STATE_ATTACHED &&
                !session->views_invalidated &&
                session->acquired_frame_count == 0 &&
                session->active_views == 0;
    if (detaching) {
      session->state = MLN_RENDER_SESSION_STATE_DETACHING;
      ++session->generation;
      session->driver_call_in_flight = true;
      session->driver_call_thread = current_owner_thread();
      discarded.swap(session->driver_work);
    }
  }
  auto status = MLN_STATUS_NATIVE_ERROR;
  if (detaching) {
    // Queued work completes as abandoned before the detach, so a frame release
    // among it still reaches the backend that holds its slot.
    abandon_driver_work(discarded);
    try {
      auto detach = [&]() -> mln_status { return finish_detach(session); };
      status = mln::c_api::with_autorelease_pool(detach);
    } catch (...) {
      status = MLN_STATUS_NATIVE_ERROR;
    }
  }
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    if (detaching) {
      session->driver_call_in_flight = false;
      session->driver_call_thread.reset();
      // Work the detach itself queued, such as a demand resumed by a frame
      // release, targets nothing now.
      discarded.swap(session->driver_work);
    }
    session->views_invalidated = true;
    session->stop_worker = true;
    session->worker_condition.notify_all();
  }
  abandon_driver_work(discarded);
  if (status == MLN_STATUS_OK)
    mln::testing::hit(mln::testing::SyncPoint::RenderDisposalDetached);
  session_teardown_lane().submit(session->disposal_task);
}

// Whether disposal may detach an eligible session and destroy the graphics
// objects of the others as abandon does, or keeps the graphics objects of
// every session, which makes no graphics call.
enum class DisposalTeardown : std::uint8_t { DetachOnWorker, Quarantine };

// Disposal owns an embedded node and a self-reference.
// Unless the caller asks for quarantine, an attached core-worker session with
// no acquired frame detaches on its worker, which then hands it to the
// teardown lane. Any other session goes to the lane directly. A driver call or
// a borrowed view still running when the lane reaches the node parks it on the
// session, and the call or view that leaves the session idle returns it to the
// lane, which abandons the target outside registry locks.
// Returns false, accepting nothing, when another disposal of the session came
// first.
auto dispose_render_session(
  const std::shared_ptr<mln_render_session_object>& live,
  DisposalTeardown teardown, const CompletionOperation& async = {}
) noexcept -> bool {
  auto detach_on_worker = false;
  {
    const auto lock = std::scoped_lock{live->control_mutex};
    if (live->disposal_requested.exchange(true)) return false;
    live->disposal_keeps_graphics = teardown == DisposalTeardown::Quarantine;
    // Accepted before the retirement is queued, so the lane delivers it.
    live->disposal_completion = async.operation;
    if (async.completion != nullptr) async.completion->accept();
    detach_on_worker =
      teardown == DisposalTeardown::DetachOnWorker &&
      live->capabilities.driver == MLN_RENDER_DRIVER_CORE_WORKER &&
      live->state == MLN_RENDER_SESSION_STATE_ATTACHED &&
      !live->views_invalidated && live->acquired_frame_count == 0 &&
      live->active_views == 0;
    if (detach_on_worker) {
      live->disposal_detach = true;
    } else {
      live->views_invalidated = true;
      live->stop_worker = true;
    }
    live->disposal_owner = live;
    live->disposal_task.context = live.get();
    live->disposal_task.run = [](RetirementTask* task) noexcept {
      auto& session = *static_cast<mln_render_session_object*>(task->context);
      if (park_retirement_while_busy(session, *task)) return;
      auto owned = session.disposal_owner;
      auto result = mln_render_abandon_result{
        .size = sizeof(mln_render_abandon_result),
        .disposition = MLN_RENDER_ABANDON_DISPOSITION_CLEAN,
        .quarantined_resource_count = 0,
        .reserved = 0,
      };
      static_cast<void>(abandon_render_session(
        owned, &result,
        session.disposal_keeps_graphics ? AbandonedGraphics::Keep
                                        : AbandonedGraphics::DestroyWhenSafe
      ));
      static_cast<void>(destroy_render_session(owned));
      // Taken before the owner goes, which can destroy the session.
      auto completion = std::move(session.disposal_completion);
      session.disposal_owner.reset();
      owned.reset();
      if (completion != nullptr) completion->complete(MLN_STATUS_OK, {}, {});
    };
    live->worker_condition.notify_all();
  }
  map_dispose_render_target_session(live->map, live.get());
  if (!detach_on_worker) session_teardown_lane().submit(live->disposal_task);
  return true;
}

auto disposal_raced(mln_render_session session) -> mln_status {
  return report_handle_fault(
    HandleTraits<mln_render_session_object>::kind, session, HandleFault::Stale
  );
}
}  // namespace

auto render_session_dispose(
  mln_render_session session, const mln_completion* completion
) -> mln_status {
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  auto async = CompletionOperation{};
  if (completion != nullptr) {
    const auto status = create_completion_operation(
      completion, valueless_completion<&mln_render_session_dispose>(), async
    );
    if (status != MLN_STATUS_OK) return status;
  }
  if (!dispose_render_session(live, DisposalTeardown::DetachOnWorker, async))
    return disposal_raced(session);
  return MLN_STATUS_OK;
}

auto render_session_dispose_quarantined(mln_render_session session)
  -> mln_status {
  const auto live = lease_render_session(session);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!dispose_render_session(live, DisposalTeardown::Quarantine))
    return disposal_raced(session);
  return MLN_STATUS_OK;
}

auto acquired_frame_discard(mln_acquired_frame frame) -> mln_status {
  auto live = handle_table<mln_acquired_frame_object>().lease(frame);
  if (live == nullptr) return recorded_handle_fault_status();
  auto expected = true;
  if (!live->valid.compare_exchange_strong(expected, false)) {
    return report_handle_fault(
      HandleTraits<mln_acquired_frame_object>::kind, frame, HandleFault::Stale
    );
  }
  static_cast<void>(handle_table<mln_acquired_frame_object>().remove(frame));
  auto& session = *live->session;
  auto wakes = DeferredWakes{};
  const auto lock = std::scoped_lock{session.control_mutex};
  if (session.acquired_frame_count) --session.acquired_frame_count;
  // Without consumer synchronization the host's GPU may still be reading the
  // slot's texture, so the slot never takes another frame. A view already open
  // on the frame keeps reading it, since the ring keeps the texture.
  if (live->slot >= session.texture.slots.size()) return MLN_STATUS_OK;
  auto& slot = session.texture.slots[live->slot];
  slot = RenderTextureSlot{};
  quarantine_slot_locked(session, slot, wakes);
  return MLN_STATUS_OK;
}

}  // namespace mln::core
