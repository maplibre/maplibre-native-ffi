#define MLN_BUILDING_C

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "map/map.hpp"

#include "bytes/buffer.hpp"
#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"

auto mln_map_options_default(void) noexcept -> mln_map_options {
  return mln::core::map_options_default();
}

auto mln_map_create(
  mln_runtime runtime, const mln_map_options* options,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::create_map_start(runtime, options, completion);
  });
}

auto mln_map_release(mln_map map) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::release_map(map);
  });
}

auto mln_map_snapshot_get(mln_map map, mln_map_snapshot* out_snapshot) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_snapshot_get(map, out_snapshot);
  });
}

auto mln_map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_resize(map, extent, completion);
  });
}

auto mln_map_request_repaint(
  mln_map map, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_request_repaint(map, completion);
  });
}

auto mln_map_request_still_image(
  mln_map map, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_request_still_image_start(map, completion);
  });
}

auto mln_map_set_event_mask(
  mln_map map, uint64_t mask, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_set_event_mask(map, mask, completion);
  });
}
