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
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"

auto mln_map_options_default(void) noexcept -> mln_map_options {
  return mln::core::map_options_default();
}

auto mln_map_create_start(
  mln_runtime runtime, const mln_map_options* options,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::create_map_start(runtime, options, out_operation);
  });
}

auto mln_map_create_take_result(
  mln_operation operation, mln_map* out_map
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::create_map_take_result(operation, out_map);
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
  mln_map map, mln_logical_extent extent, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_resize(map, extent, out_command_id);
  });
}

auto mln_map_request_repaint(mln_map map, uint64_t* out_command_id) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_request_repaint(map, out_command_id);
  });
}

auto mln_map_request_still_image_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_request_still_image_start(map, out_operation);
  });
}

auto mln_map_set_event_mask(
  mln_map map, uint64_t mask, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_set_event_mask(map, mask, out_command_id);
  });
}
