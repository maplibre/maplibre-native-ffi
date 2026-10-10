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
#include "execution/process_exit.hpp"
#include "geojson/geojson_source_data.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
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

  explicit OwnedGeoJSONOptions(const mln_geojson_source_options* source)
      : value(
          source == nullptr ? mln::core::geojson_source_options_default()
                            : *source
        ),
        cluster_properties(value.cluster_properties) {}

  [[nodiscard]] auto options() const -> mln_geojson_source_options {
    auto copied = value;
    copied.cluster_properties = cluster_properties.view();
    return copied;
  }
};

struct OwnedTileOptions {
  mln_style_tile_source_options value{};
  OwnedView attribution;

  explicit OwnedTileOptions(const mln_style_tile_source_options* source)
      : value(
          source == nullptr ? mln::core::style_tile_source_options_default()
                            : *source
        ),
        attribution(value.attribution) {}

  [[nodiscard]] auto options() const -> mln_style_tile_source_options {
    auto copied = value;
    copied.attribution = attribution.view();
    return copied;
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
template <typename Options>
struct OwnedCallbackSourceOptions {
  enum class Ownership : std::uint8_t {
    pending,
    accepted,
    adopted,
  };

  Options value{};
  std::atomic<Ownership> ownership = Ownership::pending;

  explicit OwnedCallbackSourceOptions(const Options& options)
      : value(options) {}

  ~OwnedCallbackSourceOptions() {
    if (
      ownership.load(std::memory_order_acquire) == Ownership::accepted &&
      value.release_user_data != nullptr && !mln::core::process_exiting()
    ) {
      try {
        value.release_user_data(value.user_data);
      } catch (...) {
      }
    }
  }
};

// Checks that a view's bytes are readable: a null pointer is valid only with
// size 0. The diagnostic names the view.
auto valid_view(mln_buffer_view view, const char* name) -> bool {
  if (view.data == nullptr && view.size != 0) {
    mln::core::set_thread_error((std::string{name} + " is invalid").c_str());
    return false;
  }
  return true;
}

// Checks a view that must name at least one byte, and records which way it
// failed. Every ID, URL, and JSON input takes this check before submission, so
// the map worker never sees an empty one.
auto valid_non_empty_view(mln_buffer_view view, const char* name) -> bool {
  if (view.size == 0) {
    mln::core::set_thread_error(
      (std::string{name} + " must not be empty").c_str()
    );
    return false;
  }
  return valid_view(view, name);
}

// Checks a tile URL list: at least one entry, each naming at least one byte.
auto valid_tile_urls(const mln_buffer_view* tiles, size_t tile_count) -> bool {
  if (tile_count == 0) {
    mln::core::set_thread_error("tile_count must be greater than 0");
    return false;
  }
  if (tiles == nullptr) {
    mln::core::set_thread_error("tiles must not be null");
    return false;
  }
  for (size_t index = 0; index < tile_count; ++index) {
    if (!valid_non_empty_view(tiles[index], "tile URL")) {
      return false;
    }
  }
  return true;
}

auto take_buffer(mln_buffer buffer, std::string& out) -> mln_status {
  mln_buffer_view view{};
  const auto status = mln::core::buffer_get(buffer, &view);
  if (status == MLN_STATUS_OK) {
    const auto* bytes = static_cast<const char*>(view.data);
    out.assign(bytes == nullptr ? "" : bytes, view.size);
  }
  mln::core::buffer_destroy(buffer);
  return status;
}

// Adds one callback-backed source. An accepted command hands user_data to the
// shared state, which releases it once the last reference drops unless the core
// call adopted the callbacks into the style. A rejected submission never
// referenced user_data, so the caller keeps it.
template <typename Options>
auto add_callback_source(
  mln_map map, mln_buffer_view source_id, const Options* options,
  mln_status (*validate)(const Options*),
  mln_status (*add)(mln::core::MapObject&, mln_buffer_view, const Options*),
  const mln_completion* completion
) -> mln_status {
  using Owned = OwnedCallbackSourceOptions<Options>;
  if (
    !valid_non_empty_view(source_id, "source_id") ||
    validate(options) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto id = OwnedView{source_id};
  auto owned = std::make_shared<Owned>(*options);
  const auto status = mln::core::submit_map_command(
    map,
    [id = std::move(id), owned, add](mln::core::MapObject& live) -> mln_status {
      const auto add_status = add(live, id.view(), &owned->value);
      if (add_status == MLN_STATUS_OK) {
        owned->ownership.store(
          Owned::Ownership::adopted, std::memory_order_release
        );
      }
      return add_status;
    },
    completion
  );
  if (status == MLN_STATUS_OK) {
    // The command body may already have run and adopted the callbacks.
    auto expected = Owned::Ownership::pending;
    static_cast<void>(owned->ownership.compare_exchange_strong(
      expected, Owned::Ownership::accepted, std::memory_order_acq_rel
    ));
  }
  return status;
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

auto mln_custom_mvt_vector_source_options_default(void) noexcept
  -> mln_custom_mvt_vector_source_options {
  return mln::core::custom_mvt_vector_source_options_default();
}

auto mln_premultiplied_rgba8_image_default(void) noexcept
  -> mln_premultiplied_rgba8_image {
  return mln::core::premultiplied_rgba8_image_default();
}

auto mln_style_image_options_default(void) noexcept -> mln_style_image_options {
  return mln::core::style_image_options_default();
}

auto mln_style_transition_options_default(void) noexcept
  -> mln_style_transition_options {
  return mln::core::style_transition_options_default();
}
auto mln_map_set_style_url(
  mln_map map, const char* url, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (url == nullptr) {
      mln::core::set_thread_error("url must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto owned = std::string{url};
    return mln::core::submit_map_command(
      map,
      [owned = std::move(owned)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_style_url(live, owned.c_str());
      },
      completion
    );
  });
}

auto mln_map_set_style_json(
  mln_map map, mln_buffer_view json, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(json, "style JSON")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto owned = OwnedView{json};
    return mln::core::submit_map_command(
      map,
      [owned = std::move(owned)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_style_json(live, owned.view());
      },
      completion
    );
  });
}

auto mln_map_get_loaded_style_json(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_loaded_style_json_start(map, completion);
  });
}

auto mln_map_get_style_url(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_style_url_start(map, completion);
  });
}

auto mln_map_add_style_source_json(
  mln_map map, mln_buffer_view source_id, mln_buffer_view source_json,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_non_empty_view(source_json, "source_json")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto json = OwnedView{source_json};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       json = std::move(json)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_add_style_source_json(
          live, id.view(), json.view()
        );
      },
      completion
    );
  });
}

auto mln_map_remove_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_remove_style_source(live, id.view());
      },
      completion
    );
  });
}

auto mln_map_get_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::start_style_operation<&mln_map_get_style_source>(
      map,
      [id = std::move(id)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        result.found =
          mln::core::map_get_style_source(live, id.view(), result.source);
        return MLN_STATUS_OK;
      },
      completion
    );
  });
}

auto mln_map_list_style_sources(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::start_style_operation<&mln_map_list_style_sources>(
      map,
      [](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        return mln::core::map_list_style_sources(live, result.sources);
      },
      completion
    );
  });
}

#define MLN_TILE_URL_COMMAND(NAME, CORE, KIND)                               \
  auto NAME(                                                                 \
    mln_map map, mln_buffer_view source_id, mln_buffer_view url,             \
    const mln_style_tile_source_options* options,                            \
    const mln_completion* completion, mln_diagnostic* out_diagnostic         \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status { \
      if (                                                                   \
        !valid_non_empty_view(source_id, "source_id") ||                     \
        !valid_non_empty_view(url, "url") ||                                 \
        mln::core::validate_tile_command_options(options, KIND) !=           \
          MLN_STATUS_OK                                                      \
      ) {                                                                    \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      auto id = OwnedView{source_id};                                        \
      auto owned_url = OwnedView{url};                                       \
      auto owned_options = OwnedTileOptions{options};                        \
      return mln::core::submit_map_command(                                  \
        map,                                                                 \
        [id = std::move(id), url = std::move(owned_url),                     \
         options = std::move(owned_options)](mln::core::MapObject& live)     \
          -> mln_status {                                                    \
          const auto source_options = options.options();                     \
          return mln::core::CORE(                                            \
            live, id.view(), url.view(), &source_options                     \
          );                                                                 \
        },                                                                   \
        completion                                                           \
      );                                                                     \
    });                                                                      \
  }

#define MLN_TILE_LIST_COMMAND(NAME, CORE, KIND)                              \
  auto NAME(                                                                 \
    mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,    \
    size_t tile_count, const mln_style_tile_source_options* options,         \
    const mln_completion* completion, mln_diagnostic* out_diagnostic         \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status { \
      if (                                                                   \
        !valid_non_empty_view(source_id, "source_id") ||                     \
        !valid_tile_urls(tiles, tile_count) ||                               \
        mln::core::validate_tile_command_options(options, KIND) !=           \
          MLN_STATUS_OK                                                      \
      ) {                                                                    \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      auto id = OwnedView{source_id};                                        \
      auto owned_tiles = std::vector<OwnedView>{};                           \
      owned_tiles.reserve(tile_count);                                       \
      for (size_t index = 0; index < tile_count; ++index) {                  \
        owned_tiles.emplace_back(tiles[index]);                              \
      }                                                                      \
      auto owned_options = OwnedTileOptions{options};                        \
      return mln::core::submit_map_command(                                  \
        map,                                                                 \
        [id = std::move(id), tiles = std::move(owned_tiles),                 \
         options = std::move(owned_options)](mln::core::MapObject& live)     \
          -> mln_status {                                                    \
          auto views = std::vector<mln_buffer_view>{};                       \
          views.reserve(tiles.size());                                       \
          for (const auto& tile : tiles) {                                   \
            views.push_back(tile.view());                                    \
          }                                                                  \
          const auto source_options = options.options();                     \
          return mln::core::CORE(                                            \
            live, id.view(), views.data(), views.size(), &source_options     \
          );                                                                 \
        },                                                                   \
        completion                                                           \
      );                                                                     \
    });                                                                      \
  }

auto mln_map_add_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_non_empty_view(url, "url") ||
      mln::core::validate_geojson_command_options(options) != MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned_url = OwnedView{url};
    auto owned_options = OwnedGeoJSONOptions{options};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), url = std::move(owned_url),
       options =
         std::move(owned_options)](mln::core::MapObject& live) -> mln_status {
        const auto source_options = options.options();
        return mln::core::map_add_geojson_source_url(
          live, id.view(), url.view(), &source_options
        );
      },
      completion
    );
  });
}

auto mln_geojson_source_data_create(
  mln_buffer_view data, const mln_geojson_source_options* options,
  mln_geojson_source_data* out_data, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::geojson_source_data_create(data, options, out_data);
  });
}

auto mln_geojson_source_data_destroy(mln_geojson_source_data data) noexcept
  -> void {
  mln::core::geojson_source_data_destroy(data);
}

auto mln_map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    // The lease taken at submit keeps the prepared index alive until the
    // command runs, so the host may destroy the handle right after this call.
    auto prepared = mln::core::geojson_source_data_table().lease(data);
    if (prepared == nullptr) return mln::core::recorded_handle_fault_status();
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       prepared =
         std::shared_ptr<const mln::core::GeoJsonSourceDataObject>{
           std::move(prepared)
         }](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_add_geojson_source_data(
          live, id.view(), prepared
        );
      },
      completion
    );
  });
}

auto mln_map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_non_empty_view(url, "url")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto value = OwnedView{url};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       value = std::move(value)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_geojson_source_url(
          live, id.view(), value.view()
        );
      },
      completion
    );
  });
}

auto mln_map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    // The lease taken at submit keeps the prepared index alive until the
    // command runs, so the host may destroy the handle right after this call.
    auto prepared = mln::core::geojson_source_data_table().lease(data);
    if (prepared == nullptr) return mln::core::recorded_handle_fault_status();
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       prepared =
         std::shared_ptr<const mln::core::GeoJsonSourceDataObject>{
           std::move(prepared)
         }](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_geojson_source_data(
          live, id.view(), prepared
        );
      },
      completion
    );
  });
}

auto mln_map_set_geojson_source_synchronous_tiling(
  mln_map map, mln_buffer_view source_id, bool enabled,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), enabled](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_geojson_source_synchronous_tiling(
          live, id.view(), enabled
        );
      },
      completion
    );
  });
}

auto mln_map_set_style_source_volatile(
  mln_map map, mln_buffer_view source_id, bool is_volatile,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       is_volatile](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_style_source_volatile(
          live, id.view(), is_volatile
        );
      },
      completion
    );
  });
}

MLN_TILE_URL_COMMAND(
  mln_map_add_vector_source_url, map_add_vector_source_url,
  mln::core::TileSourceOptionKind::Vector
)
MLN_TILE_LIST_COMMAND(
  mln_map_add_vector_source_tiles, map_add_vector_source_tiles,
  mln::core::TileSourceOptionKind::Vector
)
MLN_TILE_URL_COMMAND(
  mln_map_add_raster_source_url, map_add_raster_source_url,
  mln::core::TileSourceOptionKind::Raster
)
MLN_TILE_LIST_COMMAND(
  mln_map_add_raster_source_tiles, map_add_raster_source_tiles,
  mln::core::TileSourceOptionKind::Raster
)
MLN_TILE_URL_COMMAND(
  mln_map_add_raster_dem_source_url, map_add_raster_dem_source_url,
  mln::core::TileSourceOptionKind::RasterDEM
)
MLN_TILE_LIST_COMMAND(
  mln_map_add_raster_dem_source_tiles, map_add_raster_dem_source_tiles,
  mln::core::TileSourceOptionKind::RasterDEM
)

auto mln_map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return add_callback_source(
      map, source_id, options,
      mln::core::validate_custom_geometry_command_options,
      mln::core::map_add_custom_geometry_source, completion
    );
  });
}

auto mln_map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_non_empty_view(data, "data")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = OwnedView{data};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), owned = std::move(owned),
       tile_id](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_custom_geometry_source_tile_data(
          live, id.view(), tile_id, owned.view()
        );
      },
      completion
    );
  });
}

auto mln_map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), tile_id](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_invalidate_custom_geometry_source_tile(
          live, id.view(), tile_id
        );
      },
      completion
    );
  });
}

auto mln_map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), bounds](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_invalidate_custom_geometry_source_region(
          live, id.view(), bounds
        );
      },
      completion
    );
  });
}

auto mln_map_add_custom_mvt_vector_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_mvt_vector_source_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return add_callback_source(
      map, source_id, options, mln::core::validate_custom_mvt_command_options,
      mln::core::map_add_custom_mvt_vector_source, completion
    );
  });
}

auto mln_map_set_custom_mvt_vector_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") || !valid_view(data, "data")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = OwnedView{data};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), owned = std::move(owned),
       tile_id](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_custom_mvt_vector_source_tile_data(
          live, id.view(), tile_id, owned.view()
        );
      },
      completion
    );
  });
}

auto mln_map_set_custom_mvt_vector_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_view(message, "message")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = OwnedView{message};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), owned = std::move(owned),
       tile_id](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_custom_mvt_vector_source_tile_error(
          live, id.view(), tile_id, owned.view()
        );
      },
      completion
    );
  });
}

auto mln_map_invalidate_custom_mvt_vector_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), tile_id](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_invalidate_custom_mvt_vector_source_tile(
          live, id.view(), tile_id
        );
      },
      completion
    );
  });
}

#undef MLN_TILE_LIST_COMMAND
#undef MLN_TILE_URL_COMMAND

auto mln_map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(image_id, "image_id") ||
      mln::core::validate_style_image_command_input(image, options) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{image_id};
    auto owned_image = OwnedImage{image};
    auto owned_options = OwnedImageOptions{options};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), image = std::move(owned_image),
       options = std::move(owned_options)](mln::core::MapObject& live) mutable
        -> mln_status {
        image.value.pixels = image.pixels.data();
        options.value.stretch_x = options.stretch_x.data();
        options.value.stretch_y = options.stretch_y.data();
        return mln::core::map_set_style_image(
          live, id.view(), &image.value, &options.value
        );
      },
      completion
    );
  });
}

auto mln_map_remove_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(image_id, "image_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{image_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_remove_style_image(live, id.view());
      },
      completion
    );
  });
}

auto mln_map_get_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(image_id, "image_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{image_id};
    return mln::core::start_style_operation<&mln_map_get_style_image>(
      map,
      [id = std::move(id)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        result.found =
          mln::core::map_get_style_image(live, id.view(), result.image);
        return MLN_STATUS_OK;
      },
      completion
    );
  });
}

auto mln_map_add_image_source_url(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_non_empty_view(url, "url") ||
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
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), value = std::move(value),
       points = std::move(points)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_add_image_source_url(
          live, id.view(), points.data(), points.size(), value.view()
        );
      },
      completion
    );
  });
}

auto mln_map_add_image_source_image(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
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
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), owned = std::move(owned),
       points =
         std::move(points)](mln::core::MapObject& live) mutable -> mln_status {
        owned.value.pixels = owned.pixels.data();
        return mln::core::map_add_image_source_image(
          live, id.view(), points.data(), points.size(), &owned.value
        );
      },
      completion
    );
  });
}

auto mln_map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      !valid_non_empty_view(url, "url")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto value = OwnedView{url};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       value = std::move(value)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_image_source_url(
          live, id.view(), value.view()
        );
      },
      completion
    );
  });
}

auto mln_map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      mln::core::validate_style_image_command_input(image, nullptr) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto owned = OwnedImage{image};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       owned =
         std::move(owned)](mln::core::MapObject& live) mutable -> mln_status {
        owned.value.pixels = owned.pixels.data();
        return mln::core::map_set_image_source_image(
          live, id.view(), &owned.value
        );
      },
      completion
    );
  });
}

auto mln_map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(source_id, "source_id") ||
      mln::core::validate_image_source_command_coordinates(
        coordinates, coordinate_count
      ) != MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    auto points =
      std::vector<mln_lat_lng>(coordinates, coordinates + coordinate_count);
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       points = std::move(points)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_image_source_coordinates(
          live, id.view(), points.data(), points.size()
        );
      },
      completion
    );
  });
}

auto mln_map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(source_id, "source_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{source_id};
    return mln::core::start_style_operation<
      &mln_map_get_image_source_coordinates>(
      map,
      [id = std::move(id)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        return mln::core::map_get_image_source_coordinates(
          live, id.view(), result.coordinates, &result.found
        );
      },
      completion
    );
  });
}

#define MLN_LAYER_THREE_VIEW_COMMAND(NAME, CORE)                             \
  auto NAME(                                                                 \
    mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,        \
    mln_buffer_view before_layer_id, const mln_completion* completion,       \
    mln_diagnostic* out_diagnostic                                           \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status { \
      if (                                                                   \
        !valid_non_empty_view(layer_id, "layer_id") ||                       \
        !valid_non_empty_view(source_id, "source_id") ||                     \
        !valid_view(before_layer_id, "before_layer_id")                      \
      ) {                                                                    \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      auto layer = OwnedView{layer_id};                                      \
      auto source = OwnedView{source_id};                                    \
      auto before = OwnedView{before_layer_id};                              \
      return mln::core::submit_map_command(                                  \
        map,                                                                 \
        [layer = std::move(layer), source = std::move(source),               \
         before =                                                            \
           std::move(before)](mln::core::MapObject& live) -> mln_status {    \
          return mln::core::CORE(                                            \
            live, layer.view(), source.view(), before.view()                 \
          );                                                                 \
        },                                                                   \
        completion                                                           \
      );                                                                     \
    });                                                                      \
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
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_view(before_layer_id, "before_layer_id")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto before = OwnedView{before_layer_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       before = std::move(before)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_add_location_indicator_layer(
          live, id.view(), before.view()
        );
      },
      completion
    );
  });
}

auto mln_map_set_location_indicator_location(
  mln_map map, mln_buffer_view layer_id, mln_lat_lng coordinate,
  double altitude, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      mln::core::validate_location_indicator_location_command(
        coordinate, altitude
      ) != MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), coordinate,
       altitude](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_location_indicator_location(
          live, id.view(), coordinate, altitude
        );
      },
      completion
    );
  });
}

auto mln_map_set_location_indicator_bearing(
  mln_map map, mln_buffer_view layer_id, double bearing,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      mln::core::validate_location_indicator_bearing_command(bearing) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), bearing](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_location_indicator_bearing(
          live, id.view(), bearing
        );
      },
      completion
    );
  });
}

auto mln_map_set_location_indicator_accuracy_radius(
  mln_map map, mln_buffer_view layer_id, double radius,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      mln::core::validate_location_indicator_accuracy_radius_command(radius) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), radius](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_location_indicator_accuracy_radius(
          live, id.view(), radius
        );
      },
      completion
    );
  });
}

auto mln_map_set_location_indicator_image_name(
  mln_map map, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_non_empty_view(image_id, "image_id") ||
      mln::core::validate_location_indicator_image_kind(image_kind) !=
        MLN_STATUS_OK
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto layer = OwnedView{layer_id};
    auto image = OwnedView{image_id};
    return mln::core::submit_map_command(
      map,
      [layer = std::move(layer), image = std::move(image),
       image_kind](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_location_indicator_image_name(
          live, layer.view(), image_kind, image.view()
        );
      },
      completion
    );
  });
}
auto mln_map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json, mln_buffer_view before_layer_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_json, "layer_json") ||
      !valid_view(before_layer_id, "before_layer_id")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto json = OwnedView{layer_json};
    auto before = OwnedView{before_layer_id};
    return mln::core::submit_map_command(
      map,
      [json = std::move(json),
       before = std::move(before)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_add_style_layer_json(
          live, json.view(), before.view()
        );
      },
      completion
    );
  });
}

auto mln_map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(layer_id, "layer_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_remove_style_layer(live, id.view());
      },
      completion
    );
  });
}

auto mln_map_get_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(layer_id, "layer_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return mln::core::start_style_operation<&mln_map_get_style_layer>(
      map,
      [id = std::move(id)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        result.found =
          mln::core::map_get_style_layer(live, id.view(), result.layer);
        return MLN_STATUS_OK;
      },
      completion
    );
  });
}

auto mln_map_list_style_layers(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::start_style_operation<&mln_map_list_style_layers>(
      map,
      [](mln::core::MapObject& live, mln::core::StyleOperationResult& result) {
        return mln::core::map_list_style_layers(live, result.layers);
      },
      completion
    );
  });
}

auto mln_map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_view(before_layer_id, "before_layer_id")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto before = OwnedView{before_layer_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       before = std::move(before)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_move_style_layer(live, id.view(), before.view());
      },
      completion
    );
  });
}

auto mln_map_get_style_layer_json(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(layer_id, "layer_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    return mln::core::start_style_operation<&mln_map_get_style_layer_json>(
      map,
      [id = std::move(id)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        auto buffer = mln_buffer{MLN_HANDLE_NULL};
        const auto status = mln::core::map_get_style_layer_json(
          live, id.view(), &buffer, &result.found
        );
        return status == MLN_STATUS_OK && result.found
                 ? take_buffer(buffer, result.bytes)
                 : status;
      },
      completion
    );
  });
}

auto mln_map_set_style_light_json(
  mln_map map, mln_buffer_view light_json, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(light_json, "light_json")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto json = OwnedView{light_json};
    return mln::core::submit_map_command(
      map,
      [json = std::move(json)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_style_light_json(live, json.view());
      },
      completion
    );
  });
}

auto mln_map_set_global_state_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_view(property_name, "property_name") ||
      !valid_non_empty_view(value, "value")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto name = OwnedView{property_name};
    auto json = OwnedView{value};
    return mln::core::submit_map_command(
      map,
      [name = std::move(name),
       json = std::move(json)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_global_state_property(
          live, name.view(), json.view()
        );
      },
      completion
    );
  });
}

auto mln_map_get_global_state(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::start_style_operation<&mln_map_get_global_state>(
      map,
      [](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        auto buffer = mln_buffer{MLN_HANDLE_NULL};
        const auto status = mln::core::map_get_global_state(live, &buffer);
        if (status != MLN_STATUS_OK) return status;
        return take_buffer(buffer, result.bytes);
      },
      completion
    );
  });
}

auto mln_map_set_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(property_name, "property_name") ||
      !valid_non_empty_view(value, "value")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto name = OwnedView{property_name};
    auto json = OwnedView{value};
    return mln::core::submit_map_command(
      map,
      [name = std::move(name),
       json = std::move(json)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_style_light_property(
          live, name.view(), json.view()
        );
      },
      completion
    );
  });
}

auto mln_map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(property_name, "property_name")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto name = OwnedView{property_name};
    return mln::core::start_style_operation<&mln_map_get_style_light_property>(
      map,
      [name = std::move(name)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        auto buffer = mln_buffer{MLN_HANDLE_NULL};
        const auto status =
          mln::core::map_get_style_light_property(live, name.view(), &buffer);
        if (status != MLN_STATUS_OK || buffer == MLN_HANDLE_NULL) return status;
        result.found = true;
        return take_buffer(buffer, result.bytes);
      },
      completion
    );
  });
}

auto mln_map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (options == nullptr) {
      mln::core::set_thread_error("options must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (options->size < sizeof(mln_style_transition_options)) {
      mln::core::set_thread_error(
        "mln_style_transition_options.size is too small"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto owned = *options;
    return mln::core::submit_map_command(
      map,
      [owned](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_style_transition_options(live, &owned);
      },
      completion
    );
  });
}

auto mln_map_get_style_transition_options(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::start_style_operation<
      &mln_map_get_style_transition_options>(
      map,
      [](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        result.transition_options =
          mln::core::style_transition_options_default();
        return mln::core::map_get_style_transition_options(
          live, &result.transition_options
        );
      },
      completion
    );
  });
}

#define MLN_STYLE_SCALAR_COMMAND(NAME, CORE, TYPE)                           \
  auto NAME(                                                                 \
    mln_map map, mln_buffer_view layer_id, TYPE value,                       \
    const mln_completion* completion, mln_diagnostic* out_diagnostic         \
  ) noexcept -> mln_status {                                                 \
    return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status { \
      if (!valid_non_empty_view(layer_id, "layer_id")) {                     \
        return MLN_STATUS_INVALID_ARGUMENT;                                  \
      }                                                                      \
      auto owned = OwnedView{layer_id};                                      \
      return mln::core::submit_map_command(                                  \
        map,                                                                 \
        [owned = std::move(owned),                                           \
         value](mln::core::MapObject& live) -> mln_status {                  \
          return mln::core::CORE(live, owned.view(), value);                 \
        },                                                                   \
        completion                                                           \
      );                                                                     \
    });                                                                      \
  }

auto mln_map_set_style_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_non_empty_view(property_name, "property_name") ||
      !valid_non_empty_view(value, "value")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto name = OwnedView{property_name};
    auto json = OwnedView{value};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id), name = std::move(name),
       json = std::move(json)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_layer_property(
          live, id.view(), name.view(), json.view()
        );
      },
      completion
    );
  });
}

auto mln_map_get_style_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_non_empty_view(property_name, "property_name")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto name = OwnedView{property_name};
    return mln::core::start_style_operation<&mln_map_get_style_layer_property>(
      map,
      [id = std::move(id), name = std::move(name)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        auto buffer = mln_buffer{MLN_HANDLE_NULL};
        const auto status = mln::core::map_get_layer_property(
          live, id.view(), name.view(), &buffer
        );
        if (status != MLN_STATUS_OK || buffer == MLN_HANDLE_NULL) return status;
        result.found = true;
        return take_buffer(buffer, result.bytes);
      },
      completion
    );
  });
}

auto mln_map_set_style_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_buffer_view* filter,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      (filter != nullptr && !valid_non_empty_view(*filter, "filter"))
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto owned_filter = filter == nullptr
                          ? std::optional<OwnedView>{}
                          : std::optional<OwnedView>{OwnedView{*filter}};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       owned_filter =
         std::move(owned_filter)](mln::core::MapObject& live) -> mln_status {
        const auto view =
          owned_filter ? std::optional<mln_buffer_view>{owned_filter->view()}
                       : std::nullopt;
        return mln::core::map_set_layer_filter(
          live, id.view(), view ? &*view : nullptr
        );
      },
      completion
    );
  });
}

auto mln_map_get_style_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!valid_non_empty_view(layer_id, "layer_id")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto owned = OwnedView{layer_id};
    return mln::core::start_style_operation<&mln_map_get_style_layer_filter>(
      map,
      [owned = std::move(owned)](
        mln::core::MapObject& live, mln::core::StyleOperationResult& result
      ) -> mln_status {
        auto buffer = mln_buffer{MLN_HANDLE_NULL};
        const auto status =
          mln::core::map_get_layer_filter(live, owned.view(), &buffer);
        if (status != MLN_STATUS_OK) return status;
        if (buffer == MLN_HANDLE_NULL) return MLN_STATUS_OK;
        result.found = true;
        return take_buffer(buffer, result.bytes);
      },
      completion
    );
  });
}

auto mln_map_set_style_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_layer,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_view(source_layer, "source_layer")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto source = OwnedView{source_layer};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       source = std::move(source)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_layer_source_layer(
          live, id.view(), source.view()
        );
      },
      completion
    );
  });
}

auto mln_map_set_style_layer_source_id(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      !valid_non_empty_view(layer_id, "layer_id") ||
      !valid_non_empty_view(source_id, "source_id")
    ) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto id = OwnedView{layer_id};
    auto source = OwnedView{source_id};
    return mln::core::submit_map_command(
      map,
      [id = std::move(id),
       source = std::move(source)](mln::core::MapObject& live) -> mln_status {
        return mln::core::map_set_layer_source_id(
          live, id.view(), source.view()
        );
      },
      completion
    );
  });
}

MLN_STYLE_SCALAR_COMMAND(
  mln_map_set_style_layer_min_zoom, map_set_layer_min_zoom, double
)
MLN_STYLE_SCALAR_COMMAND(
  mln_map_set_style_layer_max_zoom, map_set_layer_max_zoom, double
)
MLN_STYLE_SCALAR_COMMAND(
  mln_map_set_style_layer_visibility, map_set_layer_visibility, uint32_t
)

#undef MLN_STYLE_SCALAR_COMMAND
