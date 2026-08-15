#define MLN_BUILDING_C

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "bytes/buffer.hpp"
#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"

namespace {

struct OwnedView {
  std::string bytes;

  OwnedView() = default;
  explicit OwnedView(mln_buffer_view view)
      : bytes(
          view.data == nullptr
            ? std::string{}
            : std::string{
                static_cast<const char*>(view.data),
                static_cast<const char*>(view.data) + view.size
              }
        ) {}

  [[nodiscard]] auto view() const noexcept -> mln_buffer_view {
    return {.data = bytes.data(), .size = bytes.size()};
  }
};

struct OwnedGeoJSONOptions {
  mln_geojson_source_options value{};
  OwnedView cluster_properties;

  explicit OwnedGeoJSONOptions(const mln_geojson_source_options* options)
      : value(
          options == nullptr ? mln::core::geojson_source_options_default()
                             : *options
        ),
        cluster_properties(value.cluster_properties) {
    value.cluster_properties = cluster_properties.view();
  }
};

struct OwnedTileOptions {
  mln_style_tile_source_options value{};
  OwnedView attribution;

  explicit OwnedTileOptions(const mln_style_tile_source_options* options)
      : value(
          options == nullptr ? mln::core::style_tile_source_options_default()
                             : *options
        ),
        attribution(value.attribution) {
    value.attribution = attribution.view();
  }
};

struct OwnedImage {
  mln_premultiplied_rgba8_image value{};
  std::vector<uint8_t> pixels;

  explicit OwnedImage(const mln_premultiplied_rgba8_image* image)
      : value(
          image == nullptr ? mln::core::premultiplied_rgba8_image_default()
                           : *image
        ),
        pixels(
          image == nullptr || image->pixels == nullptr
            ? std::vector<uint8_t>{}
            : std::vector<uint8_t>{
                image->pixels, image->pixels + image->byte_length
              }
        ) {
    value.pixels = pixels.data();
  }
};

struct OwnedImageOptions {
  mln_style_image_options value{};
  std::vector<mln_image_stretch> stretch_x;
  std::vector<mln_image_stretch> stretch_y;

  explicit OwnedImageOptions(const mln_style_image_options* options)
      : value(
          options == nullptr ? mln::core::style_image_options_default()
                             : *options
        ),
        stretch_x(
          value.stretch_x == nullptr
            ? std::vector<mln_image_stretch>{}
            : std::vector<
                mln_image_stretch>{value.stretch_x, value.stretch_x + value.stretch_x_count}
        ),
        stretch_y(
          value.stretch_y == nullptr
            ? std::vector<mln_image_stretch>{}
            : std::vector<mln_image_stretch>{
                value.stretch_y, value.stretch_y + value.stretch_y_count
              }
        ) {
    value.stretch_x = stretch_x.data();
    value.stretch_y = stretch_y.data();
  }
};
struct OwnedCustomGeometryOptions {
  mln_custom_geometry_source_options value{};
  std::atomic<bool> referenced = false;

  explicit OwnedCustomGeometryOptions(
    const mln_custom_geometry_source_options& options
  )
      : value(options) {}

  ~OwnedCustomGeometryOptions() {
    if (
      referenced.load(std::memory_order_acquire) &&
      value.release_user_data != nullptr
    ) {
      try {
        value.release_user_data(value.user_data);
      } catch (...) {
      }
    }
  }
};
auto valid_view(mln_buffer_view view, const char* name) -> bool {
  if (view.data == nullptr && view.size != 0) {
    mln::core::set_thread_error(name);
    return false;
  }
  return true;
}

auto command(
  mln_map map, std::function<mln_status()> work, uint64_t* out_command_id
) -> mln_status {
  return mln::core::submit_map_command(map, std::move(work), out_command_id);
}

auto operation(
  mln_map map, mln::core::StyleOperationKind kind, mln::core::StyleWork work,
  mln_operation* out_operation
) -> mln_status {
  return mln::core::start_style_operation(
    map, kind, std::move(work), out_operation
  );
}

auto take(
  mln_operation operation, mln::core::StyleOperationKind kind,
  std::function<mln_status(mln::core::StyleOperationResult&)> transfer
) -> mln_status {
  return mln::core::take_style_operation(operation, kind, std::move(transfer));
}
auto take_preserving(
  mln_operation operation, mln::core::StyleOperationKind kind,
  std::function<
    mln::core::OperationResultTransfer(mln::core::StyleOperationResult&)>
    transfer
) -> mln_status {
  return mln::core::take_operation_result<
    std::shared_ptr<mln::core::StyleOperationResult>>(
    operation, static_cast<uint32_t>(kind),
    [transfer = std::move(transfer)](
      std::shared_ptr<mln::core::StyleOperationResult>& result
    ) mutable -> mln::core::OperationResultTransfer {
      return std::invoke(std::move(transfer), *result);
    }
  );
}

using TextCopy =
  std::function<mln_status(mln_map, mln_buffer_view, char*, size_t, size_t*)>;

auto start_text_copy(
  mln_map map, mln_buffer_view id, mln::core::StyleOperationKind kind,
  TextCopy copy, mln_operation* out_operation
) -> mln_status {
  if (!valid_view(id, "style ID is invalid")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto owned = OwnedView{id};
  return operation(
    map, kind,
    [map, owned = std::move(owned),
     copy =
       std::move(copy)](mln::core::StyleOperationResult& result) -> mln_status {
      size_t size = 0;
      auto status = copy(map, owned.view(), nullptr, 0, &size);
      if (status != MLN_STATUS_OK) {
        return status;
      }
      auto bytes = std::string(size, '\0');
      status = copy(map, owned.view(), bytes.data(), bytes.size(), &size);
      return status == MLN_STATUS_OK
               ? mln::core::create_buffer(std::move(bytes), &result.buffer)
               : status;
    },
    out_operation
  );
}

auto take_text_copy(
  mln_operation operation_id, mln::core::StyleOperationKind kind,
  mln_buffer* out_value
) -> mln_status {
  if (out_value == nullptr || *out_value != MLN_HANDLE_NULL) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take(
    operation_id, kind,
    [out_value](mln::core::StyleOperationResult& result) -> mln_status {
      *out_value = std::exchange(result.buffer, MLN_HANDLE_NULL);
      return MLN_STATUS_OK;
    }
  );
}

}  // namespace

auto mln_style_tile_source_options_default(void) noexcept
  -> mln_style_tile_source_options {
  return mln::core::style_tile_source_options_default();
}

auto mln_geojson_source_options_default(void) noexcept
  -> mln_geojson_source_options {
  return mln::core::geojson_source_options_default();
}

auto mln_custom_geometry_source_options_default(void) noexcept
  -> mln_custom_geometry_source_options {
  return mln::core::custom_geometry_source_options_default();
}

auto mln_premultiplied_rgba8_image_default(void) noexcept
  -> mln_premultiplied_rgba8_image {
  return mln::core::premultiplied_rgba8_image_default();
}

auto mln_style_image_options_default(void) noexcept -> mln_style_image_options {
  return mln::core::style_image_options_default();
}

auto mln_style_image_info_default(void) noexcept -> mln_style_image_info {
  return mln::core::style_image_info_default();
}

auto mln_style_transition_options_default(void) noexcept
  -> mln_style_transition_options {
  return mln::core::style_transition_options_default();
}
auto mln_map_set_style_url(
  mln_map map, const char* url, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (url == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto owned = std::string{url};
    return command(
      map,
      [map, owned = std::move(owned)]() -> mln_status {
        return mln::core::map_set_style_url(map, owned.c_str());
      },
      out_command_id
    );
  });
}

auto mln_map_set_style_json(
  mln_map map, mln_buffer_view json, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (json.size == 0 || !valid_view(json, "style JSON is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto owned = OwnedView{json};
    return command(
      map,
      [map, owned = std::move(owned)]() -> mln_status {
        return mln::core::map_set_style_json(map, owned.view());
      },
      out_command_id
    );
  });
}

auto mln_map_loaded_style_json_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_loaded_style_json_start(map, out_operation);
  });
}

auto mln_map_loaded_style_json_take_result(
  mln_operation operation, mln_buffer* out_json
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_loaded_style_json_take_result(operation, out_json);
  });
}

auto mln_map_style_url_start(mln_map map, mln_operation* out_operation) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_style_url_start(map, out_operation);
  });
}

auto mln_map_style_url_take_result(
  mln_operation operation, mln_buffer* out_url
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_style_url_take_result(operation, out_url);
  });
}

auto mln_style_id_list_count(mln_style_id_list list, size_t* out_count) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::style_id_list_count(list, out_count);
  });
}

auto mln_style_id_list_get(
  mln_style_id_list list, size_t index, mln_buffer_view* out_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::style_id_list_get(list, index, out_id);
  });
}

auto mln_style_id_list_destroy(mln_style_id_list list) noexcept -> void {
  mln::core::style_id_list_destroy(list);
}

auto mln_style_string_list_count(
  mln_style_string_list list, size_t* out_count
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::style_string_list_count(list, out_count);
  });
}

auto mln_style_string_list_get(
  mln_style_string_list list, size_t index, mln_buffer_view* out_value
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::style_string_list_get(list, index, out_value);
  });
}

auto mln_style_string_list_destroy(mln_style_string_list list) noexcept
  -> void {
  mln::core::style_string_list_destroy(list);
}

auto mln_map_add_style_source_json(
  mln_map map, mln_buffer_view source_id, mln_buffer_view source_json,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") ||
      !valid_view(source_json, "source_json is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto json = OwnedView{source_json};
    return command(
      map,
      [map, id = std::move(id), json = std::move(json)]() -> mln_status {
        return mln::core::map_add_style_source_json(
          map, id.view(), json.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_remove_style_source_start(
  mln_map map, mln_buffer_view source_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(source_id, "source_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return operation(
      map, mln::core::StyleOperationKind::RemoveSource,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        return mln::core::map_remove_style_source(map, id.view(), &result.flag);
      },
      out_operation
    );
  });
}

auto mln_map_remove_style_source_take_result(
  mln_operation operation_id, bool* out_removed
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_removed == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::RemoveSource,
      [out_removed](mln::core::StyleOperationResult& result) -> mln_status {
        *out_removed = result.flag;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_get_style_source_info_start(
  mln_map map, mln_buffer_view source_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(source_id, "source_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return operation(
      map, mln::core::StyleOperationKind::SourceInfo,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        result.source_info = {};
        result.source_info.size = sizeof(mln_style_source_info);
        return mln::core::map_get_style_source_info(
          map, id.view(), &result.source_info, &result.found
        );
      },
      out_operation
    );
  });
}
auto mln_map_copy_style_source_attribution_start(
  mln_map map, mln_buffer_view source_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(source_id, "source_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return operation(
      map, mln::core::StyleOperationKind::SourceAttribution,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        size_t size = 0;
        auto status = mln::core::map_copy_style_source_attribution(
          map, id.view(), nullptr, 0, &size, &result.found
        );
        if (status != MLN_STATUS_OK || !result.found) {
          return status;
        }
        auto bytes = std::string(size, '\0');
        status = mln::core::map_copy_style_source_attribution(
          map, id.view(), bytes.data(), bytes.size(), &size, &result.found
        );
        return status == MLN_STATUS_OK
                 ? mln::core::create_buffer(std::move(bytes), &result.buffer)
                 : status;
      },
      out_operation
    );
  });
}

auto mln_map_copy_style_source_attribution_take_result(
  mln_operation operation_id, mln_buffer* out_attribution, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_attribution == nullptr || *out_attribution != MLN_HANDLE_NULL ||
      out_found == nullptr
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::SourceAttribution,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_attribution = std::exchange(result.buffer, MLN_HANDLE_NULL);
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_copy_style_source_url_start(
  mln_map map, mln_buffer_view source_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(source_id, "source_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return operation(
      map, mln::core::StyleOperationKind::SourceUrl,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        size_t size = 0;
        auto status = mln::core::map_copy_style_source_url(
          map, id.view(), nullptr, 0, &size, &result.found
        );
        if (status != MLN_STATUS_OK || !result.found) {
          return status;
        }
        auto bytes = std::string(size, '\0');
        status = mln::core::map_copy_style_source_url(
          map, id.view(), bytes.data(), bytes.size(), &size, &result.found
        );
        return status == MLN_STATUS_OK
                 ? mln::core::create_buffer(std::move(bytes), &result.buffer)
                 : status;
      },
      out_operation
    );
  });
}

auto mln_map_copy_style_source_url_take_result(
  mln_operation operation_id, mln_buffer* out_url, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_url == nullptr || *out_url != MLN_HANDLE_NULL || out_found == nullptr
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::SourceUrl,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_url = std::exchange(result.buffer, MLN_HANDLE_NULL);
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_get_style_source_info_take_result(
  mln_operation operation_id, mln_style_source_info* out_info, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_info == nullptr || out_info->size < sizeof(mln_style_source_info) ||
      out_found == nullptr
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::SourceInfo,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_info = result.source_info;
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_get_style_source_tile_urls_start(
  mln_map map, mln_buffer_view source_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(source_id, "source_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return operation(
      map, mln::core::StyleOperationKind::SourceTileUrls,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        return mln::core::map_get_style_source_tile_urls(
          map, id.view(), &result.string_list, &result.found
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_style_source_tile_urls_take_result(
  mln_operation operation_id, mln_style_string_list* out_tile_urls,
  bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_tile_urls == nullptr || *out_tile_urls != MLN_HANDLE_NULL ||
      out_found == nullptr
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::SourceTileUrls,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_tile_urls = std::exchange(result.string_list, MLN_HANDLE_NULL);
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_list_style_source_ids_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return operation(
      map, mln::core::StyleOperationKind::SourceIds,
      [map](mln::core::StyleOperationResult& result) -> mln_status {
        return mln::core::map_list_style_source_ids(map, &result.id_list);
      },
      out_operation
    );
  });
}

auto mln_map_list_style_source_ids_take_result(
  mln_operation operation_id, mln_style_id_list* out_source_ids
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_source_ids == nullptr || *out_source_ids != MLN_HANDLE_NULL) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::SourceIds,
      [out_source_ids](mln::core::StyleOperationResult& result) -> mln_status {
        *out_source_ids = std::exchange(result.id_list, MLN_HANDLE_NULL);
        return MLN_STATUS_OK;
      }
    );
  });
}

#define MLN_GEOJSON_COMMAND(NAME, CORE)                                       \
  auto NAME(                                                                  \
    mln_map map, mln_buffer_view source_id, mln_buffer_view input,            \
    const mln_geojson_source_options* options, uint64_t* out_command_id       \
  ) noexcept -> mln_status {                                                  \
    return mln::c_api::status_boundary([&]() -> mln_status {                  \
      if (                                                                    \
        !valid_view(source_id, "source_id is invalid") ||                     \
        !valid_view(input, "source input is invalid") ||                      \
        mln::core::validate_geojson_command_options(options) != MLN_STATUS_OK \
      ) {                                                                     \
        return MLN_STATUS_INVALID_ARGUMENT;                                   \
      }                                                                       \
      auto id = OwnedView{source_id};                                         \
      auto owned_input = OwnedView{input};                                    \
      auto owned_options = OwnedGeoJSONOptions{options};                      \
      return command(                                                         \
        map,                                                                  \
        [map, id = std::move(id), input = std::move(owned_input),             \
         options = std::move(owned_options)]() mutable -> mln_status {        \
          options.value.cluster_properties =                                  \
            options.cluster_properties.view();                                \
          return mln::core::CORE(                                             \
            map, id.view(), input.view(), &options.value                      \
          );                                                                  \
        },                                                                    \
        out_command_id                                                        \
      );                                                                      \
    });                                                                       \
  }

#define MLN_TILE_URL_COMMAND(NAME, CORE, KIND)                                \
  auto NAME(                                                                  \
    mln_map map, mln_buffer_view source_id, mln_buffer_view url,              \
    const mln_style_tile_source_options* options, uint64_t* out_command_id    \
  ) noexcept -> mln_status {                                                  \
    return mln::c_api::status_boundary([&]() -> mln_status {                  \
      if (                                                                    \
        !valid_view(source_id, "source_id is invalid") ||                     \
        !valid_view(url, "url is invalid") ||                                 \
        mln::core::validate_tile_command_options(options, KIND) !=            \
          MLN_STATUS_OK                                                       \
      ) {                                                                     \
        return MLN_STATUS_INVALID_ARGUMENT;                                   \
      }                                                                       \
      auto id = OwnedView{source_id};                                         \
      auto owned_url = OwnedView{url};                                        \
      auto owned_options = OwnedTileOptions{options};                         \
      return command(                                                         \
        map,                                                                  \
        [map, id = std::move(id), url = std::move(owned_url),                 \
         options = std::move(owned_options)]() mutable -> mln_status {        \
          options.value.attribution = options.attribution.view();             \
          return mln::core::CORE(map, id.view(), url.view(), &options.value); \
        },                                                                    \
        out_command_id                                                        \
      );                                                                      \
    });                                                                       \
  }

#define MLN_TILE_LIST_COMMAND(NAME, CORE, KIND)                           \
  auto NAME(                                                              \
    mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles, \
    size_t tile_count, const mln_style_tile_source_options* options,      \
    uint64_t* out_command_id                                              \
  ) noexcept -> mln_status {                                              \
    return mln::c_api::status_boundary([&]() -> mln_status {              \
      if (                                                                \
        !valid_view(source_id, "source_id is invalid") ||                 \
        (tile_count != 0 && tiles == nullptr) ||                          \
        mln::core::validate_tile_command_options(options, KIND) !=        \
          MLN_STATUS_OK                                                   \
      ) {                                                                 \
        return MLN_STATUS_INVALID_ARGUMENT;                               \
      }                                                                   \
      auto id = OwnedView{source_id};                                     \
      auto owned_tiles = std::vector<OwnedView>{};                        \
      owned_tiles.reserve(tile_count);                                    \
      for (size_t index = 0; index < tile_count; ++index) {               \
        if (!valid_view(tiles[index], "tile URL is invalid")) {           \
          return MLN_STATUS_INVALID_ARGUMENT;                             \
        }                                                                 \
        owned_tiles.emplace_back(tiles[index]);                           \
      }                                                                   \
      auto owned_options = OwnedTileOptions{options};                     \
      return command(                                                     \
        map,                                                              \
        [map, id = std::move(id), tiles = std::move(owned_tiles),         \
         options = std::move(owned_options)]() mutable -> mln_status {    \
          auto views = std::vector<mln_buffer_view>{};                    \
          views.reserve(tiles.size());                                    \
          for (const auto& tile : tiles) {                                \
            views.push_back(tile.view());                                 \
          }                                                               \
          options.value.attribution = options.attribution.view();         \
          return mln::core::CORE(                                         \
            map, id.view(), views.data(), views.size(), &options.value    \
          );                                                              \
        },                                                                \
        out_command_id                                                    \
      );                                                                  \
    });                                                                   \
  }

MLN_GEOJSON_COMMAND(mln_map_add_geojson_source_url, map_add_geojson_source_url)
MLN_GEOJSON_COMMAND(
  mln_map_add_geojson_source_data, map_add_geojson_source_data
)

auto mln_map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") ||
      !valid_view(url, "url is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto value = OwnedView{url};
    return command(
      map,
      [map, id = std::move(id), value = std::move(value)]() -> mln_status {
        return mln::core::map_set_geojson_source_url(
          map, id.view(), value.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_buffer_view data,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") ||
      !valid_view(data, "data is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto value = OwnedView{data};
    return command(
      map,
      [map, id = std::move(id), value = std::move(value)]() -> mln_status {
        return mln::core::map_set_geojson_source_data(
          map, id.view(), value.view()
        );
      },
      out_command_id
    );
  });
}

MLN_TILE_URL_COMMAND(
  mln_map_add_vector_source_url, map_add_vector_source_url, 0
)
MLN_TILE_LIST_COMMAND(
  mln_map_add_vector_source_tiles, map_add_vector_source_tiles, 0
)
MLN_TILE_URL_COMMAND(
  mln_map_add_raster_source_url, map_add_raster_source_url, 1
)
MLN_TILE_LIST_COMMAND(
  mln_map_add_raster_source_tiles, map_add_raster_source_tiles, 1
)
MLN_TILE_URL_COMMAND(
  mln_map_add_raster_dem_source_url, map_add_raster_dem_source_url, 2
)
MLN_TILE_LIST_COMMAND(
  mln_map_add_raster_dem_source_tiles, map_add_raster_dem_source_tiles, 2
)

auto mln_map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") ||
      mln::core::validate_custom_geometry_command_options(options) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = std::make_shared<OwnedCustomGeometryOptions>(*options);
    const auto status = command(
      map,
      [map, id = std::move(id), owned]() -> mln_status {
        const auto add_status = mln::core::map_add_custom_geometry_source(
          map, id.view(), &owned->value
        );
        if (add_status == MLN_STATUS_OK) {
          owned->referenced.store(false, std::memory_order_release);
        }
        return add_status;
      },
      out_command_id
    );
    if (status == MLN_STATUS_OK) {
      owned->referenced.store(true, std::memory_order_release);
    }
    return status;
  });
}

auto mln_map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{source_id};
    auto owned = OwnedView{data};
    return command(
      map,
      [map, id = std::move(id), owned = std::move(owned),
       tile_id]() -> mln_status {
        return mln::core::map_set_custom_geometry_source_tile_data(
          map, id.view(), tile_id, owned.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{source_id};
    return command(
      map,
      [map, id = std::move(id), tile_id]() -> mln_status {
        return mln::core::map_invalidate_custom_geometry_source_tile(
          map, id.view(), tile_id
        );
      },
      out_command_id
    );
  });
}

auto mln_map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{source_id};
    return command(
      map,
      [map, id = std::move(id), bounds]() -> mln_status {
        return mln::core::map_invalidate_custom_geometry_source_region(
          map, id.view(), bounds
        );
      },
      out_command_id
    );
  });
}

#undef MLN_TILE_LIST_COMMAND
#undef MLN_TILE_URL_COMMAND
#undef MLN_GEOJSON_COMMAND

auto mln_map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(image_id, "image_id is invalid") || image_id.size == 0 ||
      mln::core::validate_style_image_command_input(image, options) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{image_id};
    auto owned_image = OwnedImage{image};
    auto owned_options = OwnedImageOptions{options};
    return command(
      map,
      [map, id = std::move(id), image = std::move(owned_image),
       options = std::move(owned_options)]() mutable -> mln_status {
        image.value.pixels = image.pixels.data();
        options.value.stretch_x = options.stretch_x.data();
        options.value.stretch_y = options.stretch_y.data();
        return mln::core::map_set_style_image(
          map, id.view(), &image.value, &options.value
        );
      },
      out_command_id
    );
  });
}

auto mln_map_remove_style_image_start(
  mln_map map, mln_buffer_view image_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{image_id};
    return operation(
      map, mln::core::StyleOperationKind::RemoveImage,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        return mln::core::map_remove_style_image(map, id.view(), &result.flag);
      },
      out_operation
    );
  });
}

auto mln_map_remove_style_image_take_result(
  mln_operation operation_id, bool* out_removed
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_removed == nullptr) return MLN_STATUS_INVALID_ARGUMENT;
    return take(
      operation_id, mln::core::StyleOperationKind::RemoveImage,
      [out_removed](mln::core::StyleOperationResult& result) -> mln_status {
        *out_removed = result.flag;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_get_style_image_info_start(
  mln_map map, mln_buffer_view image_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{image_id};
    return operation(
      map, mln::core::StyleOperationKind::ImageInfo,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        result.image_info = mln::core::style_image_info_default();
        return mln::core::map_get_style_image_info(
          map, id.view(), &result.image_info, &result.found
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_style_image_info_take_result(
  mln_operation operation_id, mln_style_image_info* out_info, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_info == nullptr || out_info->size < sizeof(mln_style_image_info) ||
      out_found == nullptr
    )
      return MLN_STATUS_INVALID_ARGUMENT;
    return take(
      operation_id, mln::core::StyleOperationKind::ImageInfo,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_info = result.image_info;
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_copy_style_image_stretches_start(
  mln_map map, mln_buffer_view image_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{image_id};
    return operation(
      map, mln::core::StyleOperationKind::ImageStretches,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        size_t x = 0;
        size_t y = 0;
        auto status = mln::core::map_copy_style_image_stretches(
          map, id.view(), nullptr, 0, &x, nullptr, 0, &y, &result.found
        );
        if (status != MLN_STATUS_OK || !result.found) return status;
        result.stretch_x.resize(x);
        result.stretch_y.resize(y);
        return mln::core::map_copy_style_image_stretches(
          map, id.view(), result.stretch_x.data(), result.stretch_x.size(), &x,
          result.stretch_y.data(), result.stretch_y.size(), &y, &result.found
        );
      },
      out_operation
    );
  });
}

auto mln_map_copy_style_image_stretches_take_result(
  mln_operation operation_id, mln_image_stretch* out_stretch_x,
  size_t stretch_x_capacity, size_t* out_stretch_x_count,
  mln_image_stretch* out_stretch_y, size_t stretch_y_capacity,
  size_t* out_stretch_y_count, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_stretch_x_count == nullptr || out_stretch_y_count == nullptr ||
      out_found == nullptr ||
      (out_stretch_x == nullptr && stretch_x_capacity != 0) ||
      (out_stretch_y == nullptr && stretch_y_capacity != 0)
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take_preserving(
      operation_id, mln::core::StyleOperationKind::ImageStretches,
      [=](mln::core::StyleOperationResult& result)
        -> mln::core::OperationResultTransfer {
        *out_stretch_x_count = result.stretch_x.size();
        *out_stretch_y_count = result.stretch_y.size();
        *out_found = result.found;
        if (out_stretch_x == nullptr && out_stretch_y == nullptr) {
          return {MLN_STATUS_OK, false};
        }
        if (
          (out_stretch_x != nullptr &&
           stretch_x_capacity < result.stretch_x.size()) ||
          (out_stretch_y != nullptr &&
           stretch_y_capacity < result.stretch_y.size())
        ) {
          return {MLN_STATUS_INVALID_ARGUMENT, false};
        }
        if (out_stretch_x != nullptr) {
          std::ranges::copy(result.stretch_x, out_stretch_x);
        }
        if (out_stretch_y != nullptr) {
          std::ranges::copy(result.stretch_y, out_stretch_y);
        }
        return {MLN_STATUS_OK, true};
      }
    );
  });
}

auto mln_map_copy_style_image_premultiplied_rgba8_start(
  mln_map map, mln_buffer_view image_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{image_id};
    return operation(
      map, mln::core::StyleOperationKind::ImagePixels,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        size_t size = 0;
        auto status = mln::core::map_copy_style_image_premultiplied_rgba8(
          map, id.view(), nullptr, 0, &size, &result.found
        );
        if (status != MLN_STATUS_OK || !result.found) return status;
        auto bytes = std::string(size, '\0');
        status = mln::core::map_copy_style_image_premultiplied_rgba8(
          map, id.view(), reinterpret_cast<uint8_t*>(bytes.data()),
          bytes.size(), &size, &result.found
        );
        return status == MLN_STATUS_OK
                 ? mln::core::create_buffer(std::move(bytes), &result.buffer)
                 : status;
      },
      out_operation
    );
  });
}

auto mln_map_copy_style_image_premultiplied_rgba8_take_result(
  mln_operation operation_id, mln_buffer* out_pixels, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_pixels == nullptr || *out_pixels != MLN_HANDLE_NULL ||
      out_found == nullptr
    )
      return MLN_STATUS_INVALID_ARGUMENT;
    return take(
      operation_id, mln::core::StyleOperationKind::ImagePixels,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_pixels = std::exchange(result.buffer, MLN_HANDLE_NULL);
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_add_image_source_url(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") || source_id.size == 0 ||
      !valid_view(url, "url is invalid") || url.size == 0 ||
      mln::core::validate_image_source_command_coordinates(
        coordinates, coordinate_count
      ) != MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto value = OwnedView{url};
    auto points =
      std::vector<mln_lat_lng>(coordinates, coordinates + coordinate_count);
    return command(
      map,
      [map, id = std::move(id), value = std::move(value),
       points = std::move(points)]() -> mln_status {
        return mln::core::map_add_image_source_url(
          map, id.view(), points.data(), points.size(), value.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_add_image_source_image(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") || source_id.size == 0 ||
      mln::core::validate_image_source_command_coordinates(
        coordinates, coordinate_count
      ) != MLN_STATUS_OK ||
      mln::core::validate_style_image_command_input(image, nullptr) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = OwnedImage{image};
    auto points =
      std::vector<mln_lat_lng>(coordinates, coordinates + coordinate_count);
    return command(
      map,
      [map, id = std::move(id), owned = std::move(owned),
       points = std::move(points)]() mutable -> mln_status {
        owned.value.pixels = owned.pixels.data();
        return mln::core::map_add_image_source_image(
          map, id.view(), points.data(), points.size(), &owned.value
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") || source_id.size == 0 ||
      !valid_view(url, "url is invalid") || url.size == 0
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto value = OwnedView{url};
    return command(
      map,
      [map, id = std::move(id), value = std::move(value)]() -> mln_status {
        return mln::core::map_set_image_source_url(
          map, id.view(), value.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") || source_id.size == 0 ||
      mln::core::validate_style_image_command_input(image, nullptr) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = OwnedImage{image};
    return command(
      map,
      [map, id = std::move(id),
       owned = std::move(owned)]() mutable -> mln_status {
        owned.value.pixels = owned.pixels.data();
        return mln::core::map_set_image_source_image(
          map, id.view(), &owned.value
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(source_id, "source_id is invalid") || source_id.size == 0 ||
      mln::core::validate_image_source_command_coordinates(
        coordinates, coordinate_count
      ) != MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto points =
      std::vector<mln_lat_lng>(coordinates, coordinates + coordinate_count);
    return command(
      map,
      [map, id = std::move(id), points = std::move(points)]() -> mln_status {
        return mln::core::map_set_image_source_coordinates(
          map, id.view(), points.data(), points.size()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_get_image_source_coordinates_start(
  mln_map map, mln_buffer_view source_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{source_id};
    return operation(
      map, mln::core::StyleOperationKind::ImageCoordinates,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        size_t count = 0;
        auto status = mln::core::map_get_image_source_coordinates(
          map, id.view(), nullptr, 0, &count, &result.found
        );
        if (status != MLN_STATUS_OK || !result.found) return status;
        result.coordinates.resize(count);
        return mln::core::map_get_image_source_coordinates(
          map, id.view(), result.coordinates.data(), result.coordinates.size(),
          &count, &result.found
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_image_source_coordinates_take_result(
  mln_operation operation_id, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_coordinate_count == nullptr || out_found == nullptr ||
      (out_coordinates == nullptr && coordinate_capacity != 0)
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take_preserving(
      operation_id, mln::core::StyleOperationKind::ImageCoordinates,
      [=](mln::core::StyleOperationResult& result)
        -> mln::core::OperationResultTransfer {
        *out_coordinate_count = result.coordinates.size();
        *out_found = result.found;
        if (out_coordinates == nullptr) {
          return {MLN_STATUS_OK, false};
        }
        if (coordinate_capacity < result.coordinates.size()) {
          return {MLN_STATUS_INVALID_ARGUMENT, false};
        }
        std::ranges::copy(result.coordinates, out_coordinates);
        return {MLN_STATUS_OK, true};
      }
    );
  });
}

#define MLN_LAYER_THREE_VIEW_COMMAND(NAME, CORE)                      \
  auto NAME(                                                          \
    mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id, \
    mln_buffer_view before_layer_id, uint64_t* out_command_id         \
  ) noexcept -> mln_status {                                          \
    return mln::c_api::status_boundary([&]() -> mln_status {          \
      auto layer = OwnedView{layer_id};                               \
      auto source = OwnedView{source_id};                             \
      auto before = OwnedView{before_layer_id};                       \
      return command(                                                 \
        map,                                                          \
        [map, layer = std::move(layer), source = std::move(source),   \
         before = std::move(before)]() -> mln_status {                \
          return mln::core::CORE(                                     \
            map, layer.view(), source.view(), before.view()           \
          );                                                          \
        },                                                            \
        out_command_id                                                \
      );                                                              \
    });                                                               \
  }
MLN_LAYER_THREE_VIEW_COMMAND(
  mln_map_add_hillshade_layer, map_add_hillshade_layer
)
MLN_LAYER_THREE_VIEW_COMMAND(
  mln_map_add_color_relief_layer, map_add_color_relief_layer
)
#undef MLN_LAYER_THREE_VIEW_COMMAND

auto mln_map_add_location_indicator_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{layer_id};
    auto before = OwnedView{before_layer_id};
    return command(
      map,
      [map, id = std::move(id), before = std::move(before)]() -> mln_status {
        return mln::core::map_add_location_indicator_layer(
          map, id.view(), before.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_location_indicator_location(
  mln_map map, mln_buffer_view layer_id, mln_lat_lng coordinate,
  double altitude, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{layer_id};
    return command(
      map,
      [map, id = std::move(id), coordinate, altitude]() -> mln_status {
        return mln::core::map_set_location_indicator_location(
          map, id.view(), coordinate, altitude
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_location_indicator_bearing(
  mln_map map, mln_buffer_view layer_id, double bearing,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{layer_id};
    return command(
      map,
      [map, id = std::move(id), bearing]() -> mln_status {
        return mln::core::map_set_location_indicator_bearing(
          map, id.view(), bearing
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_location_indicator_accuracy_radius(
  mln_map map, mln_buffer_view layer_id, double radius, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{layer_id};
    return command(
      map,
      [map, id = std::move(id), radius]() -> mln_status {
        return mln::core::map_set_location_indicator_accuracy_radius(
          map, id.view(), radius
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_location_indicator_image_name(
  mln_map map, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto layer = OwnedView{layer_id};
    auto image = OwnedView{image_id};
    return command(
      map,
      [map, layer = std::move(layer), image = std::move(image),
       image_kind]() -> mln_status {
        return mln::core::map_set_location_indicator_image_name(
          map, layer.view(), image_kind, image.view()
        );
      },
      out_command_id
    );
  });
}
auto mln_map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json, mln_buffer_view before_layer_id,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(layer_json, "layer_json is invalid") ||
      !valid_view(before_layer_id, "before_layer_id is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto json = OwnedView{layer_json};
    auto before = OwnedView{before_layer_id};
    return command(
      map,
      [map, json = std::move(json),
       before = std::move(before)]() -> mln_status {
        return mln::core::map_add_style_layer_json(
          map, json.view(), before.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_remove_style_layer_start(
  mln_map map, mln_buffer_view layer_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(layer_id, "layer_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return operation(
      map, mln::core::StyleOperationKind::RemoveLayer,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        return mln::core::map_remove_style_layer(map, id.view(), &result.flag);
      },
      out_operation
    );
  });
}

auto mln_map_remove_style_layer_take_result(
  mln_operation operation_id, bool* out_removed
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_removed == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::RemoveLayer,
      [out_removed](mln::core::StyleOperationResult& result) -> mln_status {
        *out_removed = result.flag;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_get_style_layer_type_start(
  mln_map map, mln_buffer_view layer_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(layer_id, "layer_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return operation(
      map, mln::core::StyleOperationKind::LayerType,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        auto type = mln_buffer_view{};
        const auto status = mln::core::map_get_style_layer_type(
          map, id.view(), &type, &result.found
        );
        if (status != MLN_STATUS_OK || !result.found) {
          return status;
        }
        return mln::core::create_buffer(
          std::string{
            static_cast<const char*>(type.data),
            static_cast<const char*>(type.data) + type.size
          },
          &result.buffer
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_style_layer_type_take_result(
  mln_operation operation_id, mln_buffer* out_layer_type, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_layer_type == nullptr || *out_layer_type != MLN_HANDLE_NULL ||
      out_found == nullptr
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::LayerType,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_layer_type = std::exchange(result.buffer, MLN_HANDLE_NULL);
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_list_style_layer_ids_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return operation(
      map, mln::core::StyleOperationKind::LayerIds,
      [map](mln::core::StyleOperationResult& result) -> mln_status {
        return mln::core::map_list_style_layer_ids(map, &result.id_list);
      },
      out_operation
    );
  });
}

auto mln_map_list_style_layer_ids_take_result(
  mln_operation operation_id, mln_style_id_list* out_layer_ids
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_layer_ids == nullptr || *out_layer_ids != MLN_HANDLE_NULL) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::LayerIds,
      [out_layer_ids](mln::core::StyleOperationResult& result) -> mln_status {
        *out_layer_ids = std::exchange(result.id_list, MLN_HANDLE_NULL);
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(layer_id, "layer_id is invalid") ||
      !valid_view(before_layer_id, "before_layer_id is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto before = OwnedView{before_layer_id};
    return command(
      map,
      [map, id = std::move(id), before = std::move(before)]() -> mln_status {
        return mln::core::map_move_style_layer(map, id.view(), before.view());
      },
      out_command_id
    );
  });
}

auto mln_map_get_style_layer_json_start(
  mln_map map, mln_buffer_view layer_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(layer_id, "layer_id is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return operation(
      map, mln::core::StyleOperationKind::LayerJson,
      [map, id = std::move(id)](mln::core::StyleOperationResult& result)
        -> mln_status {
        return mln::core::map_get_style_layer_json(
          map, id.view(), &result.buffer, &result.found
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_style_layer_json_take_result(
  mln_operation operation_id, mln_buffer* out_layer, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_layer == nullptr || *out_layer != MLN_HANDLE_NULL ||
      out_found == nullptr
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::LayerJson,
      [=](mln::core::StyleOperationResult& result) -> mln_status {
        *out_layer = std::exchange(result.buffer, MLN_HANDLE_NULL);
        *out_found = result.found;
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_set_style_light_json(
  mln_map map, mln_buffer_view light_json, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(light_json, "light_json is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto json = OwnedView{light_json};
    return command(
      map,
      [map, json = std::move(json)]() -> mln_status {
        return mln::core::map_set_style_light_json(map, json.view());
      },
      out_command_id
    );
  });
}

auto mln_map_set_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(property_name, "property_name is invalid") ||
      !valid_view(value, "value is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto name = OwnedView{property_name};
    auto json = OwnedView{value};
    return command(
      map,
      [map, name = std::move(name), json = std::move(json)]() -> mln_status {
        return mln::core::map_set_style_light_property(
          map, name.view(), json.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_get_style_light_property_start(
  mln_map map, mln_buffer_view property_name, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (!valid_view(property_name, "property_name is invalid")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto name = OwnedView{property_name};
    return operation(
      map, mln::core::StyleOperationKind::LightProperty,
      [map, name = std::move(name)](mln::core::StyleOperationResult& result)
        -> mln_status {
        return mln::core::map_get_style_light_property(
          map, name.view(), &result.buffer
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_style_light_property_take_result(
  mln_operation operation_id, mln_buffer* out_value
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_value == nullptr || *out_value != MLN_HANDLE_NULL) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::LightProperty,
      [out_value](mln::core::StyleOperationResult& result) -> mln_status {
        *out_value = std::exchange(result.buffer, MLN_HANDLE_NULL);
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      options == nullptr || options->size < sizeof(mln_style_transition_options)
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto owned = *options;
    return command(
      map,
      [map, owned]() -> mln_status {
        return mln::core::map_set_style_transition_options(map, &owned);
      },
      out_command_id
    );
  });
}

auto mln_map_get_style_transition_options_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return operation(
      map, mln::core::StyleOperationKind::TransitionOptions,
      [map](mln::core::StyleOperationResult& result) -> mln_status {
        result.transition_options =
          mln::core::style_transition_options_default();
        return mln::core::map_get_style_transition_options(
          map, &result.transition_options
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_style_transition_options_take_result(
  mln_operation operation_id, mln_style_transition_options* out_options
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_options == nullptr ||
      out_options->size < sizeof(mln_style_transition_options)
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::TransitionOptions,
      [out_options](mln::core::StyleOperationResult& result) -> mln_status {
        *out_options = result.transition_options;
        return MLN_STATUS_OK;
      }
    );
  });
}

#define MLN_STYLE_BUFFER_OPERATION(NAME, CORE, KIND)                         \
  auto NAME##_start(                                                         \
    mln_map map, mln_buffer_view id, mln_operation* out_operation            \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary([&]() -> mln_status {                 \
      if (!valid_view(id, "style ID is invalid")) {                          \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      auto owned = OwnedView{id};                                            \
      return operation(                                                      \
        map, mln::core::StyleOperationKind::KIND,                            \
        [map, owned = std::move(owned)](                                     \
          mln::core::StyleOperationResult& result                            \
        ) -> mln_status {                                                    \
          return mln::core::CORE(map, owned.view(), &result.buffer);         \
        },                                                                   \
        out_operation                                                        \
      );                                                                     \
    });                                                                      \
  }                                                                          \
  auto NAME##_take_result(                                                   \
    mln_operation operation_id, mln_buffer* out_value                        \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary([&]() -> mln_status {                 \
      if (out_value == nullptr || *out_value != MLN_HANDLE_NULL) {           \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      return take(                                                           \
        operation_id, mln::core::StyleOperationKind::KIND,                   \
        [out_value](mln::core::StyleOperationResult& result) -> mln_status { \
          *out_value = std::exchange(result.buffer, MLN_HANDLE_NULL);        \
          return MLN_STATUS_OK;                                              \
        }                                                                    \
      );                                                                     \
    });                                                                      \
  }

#define MLN_STYLE_SCALAR_OPERATION(NAME, CORE, KIND, TYPE, FIELD)            \
  auto NAME##_start(                                                         \
    mln_map map, mln_buffer_view id, mln_operation* out_operation            \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary([&]() -> mln_status {                 \
      if (!valid_view(id, "style ID is invalid")) {                          \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      auto owned = OwnedView{id};                                            \
      return operation(                                                      \
        map, mln::core::StyleOperationKind::KIND,                            \
        [map, owned = std::move(owned)](                                     \
          mln::core::StyleOperationResult& result                            \
        ) -> mln_status {                                                    \
          return mln::core::CORE(map, owned.view(), &result.FIELD);          \
        },                                                                   \
        out_operation                                                        \
      );                                                                     \
    });                                                                      \
  }                                                                          \
  auto NAME##_take_result(                                                   \
    mln_operation operation_id, TYPE* out_value                              \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary([&]() -> mln_status {                 \
      if (out_value == nullptr) {                                            \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      return take(                                                           \
        operation_id, mln::core::StyleOperationKind::KIND,                   \
        [out_value](mln::core::StyleOperationResult& result) -> mln_status { \
          *out_value = result.FIELD;                                         \
          return MLN_STATUS_OK;                                              \
        }                                                                    \
      );                                                                     \
    });                                                                      \
  }

#define MLN_STYLE_SCALAR_COMMAND(NAME, CORE, TYPE)                        \
  auto NAME(                                                              \
    mln_map map, mln_buffer_view id, TYPE value, uint64_t* out_command_id \
  ) noexcept -> mln_status {                                              \
    return mln::c_api::status_boundary([&]() -> mln_status {              \
      if (!valid_view(id, "style ID is invalid")) {                       \
        return MLN_STATUS_INVALID_ARGUMENT;                               \
      }                                                                   \
      auto owned = OwnedView{id};                                         \
      return command(                                                     \
        map,                                                              \
        [map, owned = std::move(owned), value]() -> mln_status {          \
          return mln::core::CORE(map, owned.view(), value);               \
        },                                                                \
        out_command_id                                                    \
      );                                                                  \
    });                                                                   \
  }

auto mln_map_set_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(layer_id, "layer_id is invalid") ||
      !valid_view(property_name, "property_name is invalid") ||
      !valid_view(value, "value is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto name = OwnedView{property_name};
    auto json = OwnedView{value};
    return command(
      map,
      [map, id = std::move(id), name = std::move(name),
       json = std::move(json)]() -> mln_status {
        return mln::core::map_set_layer_property(
          map, id.view(), name.view(), json.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_get_layer_property_start(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(layer_id, "layer_id is invalid") ||
      !valid_view(property_name, "property_name is invalid")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto name = OwnedView{property_name};
    return operation(
      map, mln::core::StyleOperationKind::LayerProperty,
      [map, id = std::move(id), name = std::move(name)](
        mln::core::StyleOperationResult& result
      ) -> mln_status {
        return mln::core::map_get_layer_property(
          map, id.view(), name.view(), &result.buffer
        );
      },
      out_operation
    );
  });
}

auto mln_map_get_layer_property_take_result(
  mln_operation operation_id, mln_buffer* out_value
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_value == nullptr || *out_value != MLN_HANDLE_NULL) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return take(
      operation_id, mln::core::StyleOperationKind::LayerProperty,
      [out_value](mln::core::StyleOperationResult& result) -> mln_status {
        *out_value = std::exchange(result.buffer, MLN_HANDLE_NULL);
        return MLN_STATUS_OK;
      }
    );
  });
}

auto mln_map_set_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_buffer_view* filter,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      !valid_view(layer_id, "layer_id is invalid") ||
      (filter != nullptr && !valid_view(*filter, "filter is invalid"))
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto owned_filter = filter == nullptr
                          ? std::optional<OwnedView>{}
                          : std::optional<OwnedView>{OwnedView{*filter}};
    return command(
      map,
      [map, id = std::move(id),
       owned_filter = std::move(owned_filter)]() -> mln_status {
        const auto view =
          owned_filter ? std::optional<mln_buffer_view>{owned_filter->view()}
                       : std::nullopt;
        return mln::core::map_set_layer_filter(
          map, id.view(), view ? &*view : nullptr
        );
      },
      out_command_id
    );
  });
}

MLN_STYLE_BUFFER_OPERATION(
  mln_map_get_layer_filter, map_get_layer_filter, LayerFilter
)

auto mln_map_set_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_layer,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{layer_id};
    auto source = OwnedView{source_layer};
    return command(
      map,
      [map, id = std::move(id), source = std::move(source)]() -> mln_status {
        return mln::core::map_set_layer_source_layer(
          map, id.view(), source.view()
        );
      },
      out_command_id
    );
  });
}

auto mln_map_set_layer_source_id(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto id = OwnedView{layer_id};
    auto source = OwnedView{source_id};
    return command(
      map,
      [map, id = std::move(id), source = std::move(source)]() -> mln_status {
        return mln::core::map_set_layer_source_id(
          map, id.view(), source.view()
        );
      },
      out_command_id
    );
  });
}

MLN_STYLE_SCALAR_COMMAND(
  mln_map_set_layer_min_zoom, map_set_layer_min_zoom, double
)
MLN_STYLE_SCALAR_COMMAND(
  mln_map_set_layer_max_zoom, map_set_layer_max_zoom, double
)
MLN_STYLE_SCALAR_COMMAND(
  mln_map_set_layer_visibility, map_set_layer_visibility, uint32_t
)
MLN_STYLE_SCALAR_OPERATION(
  mln_map_get_layer_min_zoom, map_get_layer_min_zoom, LayerMinZoom, double,
  value_double
)
MLN_STYLE_SCALAR_OPERATION(
  mln_map_get_layer_max_zoom, map_get_layer_max_zoom, LayerMaxZoom, double,
  value_double
)
MLN_STYLE_SCALAR_OPERATION(
  mln_map_get_layer_visibility, map_get_layer_visibility, LayerVisibility,
  uint32_t, value_u32
)

#undef MLN_STYLE_SCALAR_COMMAND
#undef MLN_STYLE_SCALAR_OPERATION
#undef MLN_STYLE_BUFFER_OPERATION

auto mln_map_copy_layer_source_layer_start(
  mln_map map, mln_buffer_view layer_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return start_text_copy(
      map, layer_id, mln::core::StyleOperationKind::LayerSourceLayer,
      [](
        mln_map value_map, mln_buffer_view id, char* text, size_t capacity,
        size_t* size
      ) -> mln_status {
        return mln::core::map_copy_layer_source_layer(
          value_map, id, text, capacity, size
        );
      },
      out_operation
    );
  });
}

auto mln_map_copy_layer_source_layer_take_result(
  mln_operation operation_id, mln_buffer* out_source_layer
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return take_text_copy(
      operation_id, mln::core::StyleOperationKind::LayerSourceLayer,
      out_source_layer
    );
  });
}

auto mln_map_copy_layer_source_id_start(
  mln_map map, mln_buffer_view layer_id, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return start_text_copy(
      map, layer_id, mln::core::StyleOperationKind::LayerSourceId,
      [](
        mln_map value_map, mln_buffer_view id, char* text, size_t capacity,
        size_t* size
      ) -> mln_status {
        return mln::core::map_copy_layer_source_id(
          value_map, id, text, capacity, size
        );
      },
      out_operation
    );
  });
}

auto mln_map_copy_layer_source_id_take_result(
  mln_operation operation_id, mln_buffer* out_source_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return take_text_copy(
      operation_id, mln::core::StyleOperationKind::LayerSourceId, out_source_id
    );
  });
}