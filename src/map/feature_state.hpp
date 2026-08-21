#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <mbgl/util/feature.hpp>

#include "maplibre_native_c.h"

namespace mln {
class Renderer;
class UpdateParameters;
}  // namespace mln

namespace mln::core {

using FeatureStateLayerId = std::optional<std::string>;

struct FeatureStateLayerIdHash {
  auto operator()(const FeatureStateLayerId& id) const -> std::size_t {
    auto hash = std::hash<bool>{}(id.has_value());
    if (id.has_value()) {
      hash ^= std::hash<std::string>{}(*id) + 0x9e3779b9 + (hash << 6U) +
              (hash >> 2U);
    }
    return hash;
  }
};

using FeatureStateFeatures = mln::FeatureStates;
using FeatureStateLayers = std::unordered_map<
  FeatureStateLayerId, FeatureStateFeatures, FeatureStateLayerIdHash>;
using FeatureStateSources = std::unordered_map<std::string, FeatureStateLayers>;

struct FeatureStateSnapshot {
  FeatureStateSources sources;
};

class FeatureStateStore {
 public:
  void set(
    std::string source_id, FeatureStateLayerId source_layer_id,
    std::string feature_id, mln::FeatureState state
  );
  void remove(
    std::string source_id, FeatureStateLayerId source_layer_id,
    std::optional<std::string> feature_id, std::optional<std::string> state_key
  );
  [[nodiscard]] auto get(
    const std::string& source_id, const FeatureStateLayerId& source_layer_id,
    const std::string& feature_id
  ) const -> mln::FeatureState;
  [[nodiscard]] auto snapshot() const
    -> std::shared_ptr<const FeatureStateSnapshot>;

 private:
  void ensure_unique();
  static void prune(
    FeatureStateSources& sources, const std::string& source_id,
    const FeatureStateLayerId& source_layer_id, const std::string* feature_id
  );

  mutable std::mutex mutex_;
  std::shared_ptr<FeatureStateSnapshot> published_ =
    std::make_shared<FeatureStateSnapshot>();
};

auto validate_feature_state_selector(
  const mln_feature_state_selector* selector, bool require_feature_id
) -> mln_status;
auto selector_has_field(
  const mln_feature_state_selector& selector, uint32_t field
) -> bool;
auto optional_selector_string(
  const mln_feature_state_selector& selector, uint32_t field,
  mln_buffer_view value
) -> std::optional<std::string>;
auto feature_state_source_layer(const mln_feature_state_selector& selector)
  -> FeatureStateLayerId;

auto feature_state_needs_warmup(
  const FeatureStateSnapshot& desired,
  const std::unordered_set<std::string>& rendered_source_ids,
  const mln::UpdateParameters& update
) -> bool;

void apply_feature_state_diff(
  mln::Renderer& renderer, const mln::UpdateParameters& update,
  const FeatureStateSnapshot& desired, FeatureStateSnapshot& applied,
  const std::unordered_set<std::string>& rendered_source_ids
);

void remember_rendered_sources(
  std::unordered_set<std::string>& rendered_source_ids,
  const mln::UpdateParameters& update
);

}  // namespace mln::core
