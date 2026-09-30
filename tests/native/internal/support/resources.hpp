#pragma once

// Resource configuration helpers for the internal suite's resource cases.

#include <cstdint>

#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace mln::native_tests {

// Settles a runtime configuration command: a rejected submission leaves the
// completion with the caller, and an accepted one reports its terminal status.
inline auto commit(mln_status status, mln_test_completion& completion)
  -> mln_status {
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const auto result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

inline auto set_resource_provider(
  mln_runtime runtime, const mln_resource_provider& provider
) -> mln_status {
  auto completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_resource_provider(
      runtime, &provider, &completion.descriptor, nullptr
    ),
    completion
  );
}

inline auto clear_resource_provider(mln_runtime runtime) -> mln_status {
  auto completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_resource_provider(
      runtime, &completion.descriptor, nullptr
    ),
    completion
  );
}

inline auto set_resource_transform(
  mln_runtime runtime, const mln_resource_transform& transform
) -> mln_status {
  auto completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_resource_transform(
      runtime, &transform, &completion.descriptor, nullptr
    ),
    completion
  );
}

inline auto clear_resource_transform(mln_runtime runtime) -> mln_status {
  auto completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_resource_transform(
      runtime, &completion.descriptor, nullptr
    ),
    completion
  );
}

// Creates an offline region for `style_url` and activates its download. The
// download requests the style from a MapLibre file source thread, which runs
// the provider and transform callbacks with no live map, so runtime teardown
// can overlap them.
inline auto start_offline_download(mln_runtime runtime, const char* style_url)
  -> bool {
  const auto definition = mln_offline_region_definition{
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data = {
      .tile_pyramid = {
        .size = sizeof(mln_offline_tile_pyramid_region_definition),
        .style_url = style_url,
        .bounds =
          {
            .southwest = {.latitude = 1.0, .longitude = 2.0},
            .northeast = {.latitude = 3.0, .longitude = 4.0},
          },
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
        .include_ideographs = true,
      },
    },
  };
  const std::uint8_t metadata[] = {1, 2, 3};
  auto creation = mln_test_completion_default(sizeof(mln_offline_region_info));
  if (
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &creation.descriptor,
      nullptr
    ) != MLN_STATUS_OK
  ) {
    mln_test_completion_reject(&creation);
    mln_test_completion_destroy(&creation);
    return false;
  }
  auto info = mln_offline_region_info{.size = sizeof(mln_offline_region_info)};
  if (
    mln_test_completion_finish_value(&creation, &info, sizeof(info)) !=
    MLN_STATUS_OK
  ) {
    return false;
  }
  const auto download = mln_test_discard_completion();
  return mln_runtime_offline_region_set_download_state(
           runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download,
           nullptr
         ) == MLN_STATUS_OK;
}

}  // namespace mln::native_tests
