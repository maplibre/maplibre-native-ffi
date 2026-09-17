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
#include "geojson/geojson.hpp"
#include "map/feature_state.hpp"
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

auto mln_map_release(mln_map map, const mln_completion* completion) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::release_map(map, completion);
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

namespace {

// Owns copies of the selector strings so a deferred command outlives the
// caller's buffers.
struct OwnedFeatureStateSelector {
  uint32_t fields = 0;
  std::string source_id;
  std::string source_layer_id;
  std::string feature_id;
  std::string state_key;

  explicit OwnedFeatureStateSelector(const mln_feature_state_selector& selector)
      : fields(selector.fields),
        source_id(copy_view(selector.source_id)),
        source_layer_id(copy_view(selector.source_layer_id)),
        feature_id(copy_view(selector.feature_id)),
        state_key(copy_view(selector.state_key)) {}

  [[nodiscard]] auto view() const -> mln_feature_state_selector {
    return mln_feature_state_selector{
      .size = sizeof(mln_feature_state_selector),
      .fields = fields,
      .source_id = {.data = source_id.data(), .size = source_id.size()},
      .source_layer_id =
        {.data = source_layer_id.data(), .size = source_layer_id.size()},
      .feature_id = {.data = feature_id.data(), .size = feature_id.size()},
      .state_key = {.data = state_key.data(), .size = state_key.size()},
    };
  }

 private:
  static auto copy_view(mln_buffer_view value) -> std::string {
    return value.data == nullptr
             ? std::string{}
             : std::string{static_cast<const char*>(value.data), value.size};
  }
};

}  // namespace

auto mln_map_set_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  mln_buffer_view state, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    const auto selector_status =
      mln::core::validate_feature_state_selector(selector, true);
    if (selector_status != MLN_STATUS_OK) {
      return selector_status;
    }
    if (state.data == nullptr || state.size == 0) {
      mln::core::set_thread_error("state must not be empty");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    // The header promises validation before return, so parse here; the
    // command parses again on the worker, which stays cheap for the small
    // objects feature state carries.
    const auto parsed = mln::core::to_native_json_value(state);
    if (!parsed || parsed->getObject() == nullptr) {
      mln::core::set_thread_error("feature state value must be a JSON object");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto owned_selector = OwnedFeatureStateSelector{*selector};
    auto owned_state =
      std::string{static_cast<const char*>(state.data), state.size};
    return mln::core::submit_map_command(
      map,
      [owned_selector = std::move(owned_selector),
       owned_state =
         std::move(owned_state)](mln::core::MapObject& live) -> mln_status {
        const auto selector = owned_selector.view();
        return mln::core::map_set_feature_state(
          live, &selector,
          {.data = owned_state.data(), .size = owned_state.size()}
        );
      },
      completion
    );
  });
}

auto mln_map_get_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_get_feature_state_start(map, selector, completion);
  });
}

auto mln_map_remove_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    const auto selector_status =
      mln::core::validate_feature_state_selector(selector, false);
    if (selector_status != MLN_STATUS_OK) {
      return selector_status;
    }
    auto owned_selector = OwnedFeatureStateSelector{*selector};
    return mln::core::submit_map_command(
      map,
      [owned_selector =
         std::move(owned_selector)](mln::core::MapObject& live) -> mln_status {
        const auto selector = owned_selector.view();
        return mln::core::map_remove_feature_state(live, &selector);
      },
      completion
    );
  });
}
