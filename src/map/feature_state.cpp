#include <algorithm>
#include <ranges>
#include <utility>

#include <mln/renderer/renderer.hpp>
#include <mln/renderer/update_parameters.hpp>
#include <mln/style/source_impl.hpp>
#include <mln/util/feature.hpp>

#include "map/feature_state.hpp"

#include "bytes/buffer.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "maplibre_native_c.h"

namespace {

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

auto string_from_view(mln_buffer_view string) -> std::string {
  if (string.size == 0) {
    return {};
  }
  return std::string{static_cast<const char*>(string.data), string.size};
}

auto update_has_source(
  const mln::UpdateParameters& update, const std::string& source_id
) -> bool {
  return std::ranges::any_of(*update.sources, [&](const auto& source) {
    return source->id == source_id;
  });
}

auto merge_state(mln::FeatureState& target, const mln::FeatureState& source)
  -> void {
  for (const auto& [key, value] : source) {
    target[key] = value;
  }
}

auto layer_or_empty(
  const mln::core::FeatureStateSnapshot& snapshot, const std::string& source_id,
  bool present
) -> const mln::core::FeatureStateLayers* {
  if (!present) {
    return nullptr;
  }
  const auto source = snapshot.sources.find(source_id);
  if (source == snapshot.sources.end()) {
    return nullptr;
  }
  return &source->second;
}

}  // namespace

namespace mln::core {

void FeatureStateStore::ensure_unique() {
  if (published_.use_count() > 1) {
    published_ = std::make_shared<FeatureStateSnapshot>(*published_);
  }
}

void FeatureStateStore::prune(
  FeatureStateSources& sources, const std::string& source_id,
  const FeatureStateLayerId& source_layer_id, const std::string* feature_id
) {
  const auto source = sources.find(source_id);
  if (source == sources.end()) {
    return;
  }
  const auto layer = source->second.find(source_layer_id);
  if (layer == source->second.end()) {
    return;
  }
  if (feature_id != nullptr) {
    const auto feature = layer->second.find(*feature_id);
    if (feature != layer->second.end() && feature->second.empty()) {
      layer->second.erase(feature);
    }
  }
  if (layer->second.empty()) {
    source->second.erase(layer);
  }
  if (source->second.empty()) {
    sources.erase(source);
  }
}

void FeatureStateStore::set(
  std::string source_id, FeatureStateLayerId source_layer_id,
  std::string feature_id, mln::FeatureState state
) {
  const std::scoped_lock lock{mutex_};
  ensure_unique();
  auto& feature =
    published_->sources[std::move(source_id)][std::move(source_layer_id)]
                       [std::move(feature_id)];
  merge_state(feature, state);
}

void FeatureStateStore::remove(
  std::string source_id, FeatureStateLayerId source_layer_id,
  std::optional<std::string> feature_id, std::optional<std::string> state_key
) {
  const std::scoped_lock lock{mutex_};
  ensure_unique();
  const auto source = published_->sources.find(source_id);
  if (source == published_->sources.end()) {
    return;
  }
  const auto layer = source->second.find(source_layer_id);
  if (layer == source->second.end()) {
    return;
  }
  if (!feature_id.has_value()) {
    source->second.erase(layer);
    if (source->second.empty()) {
      published_->sources.erase(source);
    }
    return;
  }
  const auto feature = layer->second.find(*feature_id);
  if (feature == layer->second.end()) {
    return;
  }
  if (!state_key.has_value()) {
    layer->second.erase(feature);
  } else {
    feature->second.erase(*state_key);
  }
  prune(published_->sources, source_id, source_layer_id, &*feature_id);
}

auto FeatureStateStore::get(
  const std::string& source_id, const FeatureStateLayerId& source_layer_id,
  const std::string& feature_id
) const -> mln::FeatureState {
  const std::scoped_lock lock{mutex_};
  const auto source = published_->sources.find(source_id);
  if (source == published_->sources.end()) {
    return {};
  }
  const auto layer = source->second.find(source_layer_id);
  if (layer == source->second.end()) {
    return {};
  }
  const auto feature = layer->second.find(feature_id);
  if (feature == layer->second.end()) {
    return {};
  }
  return feature->second;
}

auto FeatureStateStore::snapshot() const
  -> std::shared_ptr<const FeatureStateSnapshot> {
  const std::scoped_lock lock{mutex_};
  return published_;
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
    set_thread_error("feature state selector must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (selector->size < sizeof(mln_feature_state_selector)) {
    set_thread_error("mln_feature_state_selector.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((selector->fields & ~feature_state_selector_known_fields) != 0) {
    set_thread_error("feature state selector has unknown fields");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!validate_string_view(selector->source_id)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (selector->source_id.size == 0) {
    set_thread_error("feature state source_id must not be empty");
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
    set_thread_error("feature state selector requires feature_id");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    selector_has_field(*selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY) &&
    !has_feature_id
  ) {
    set_thread_error("feature state selector state_key requires feature_id");
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
  -> FeatureStateLayerId {
  return optional_selector_string(
    selector, MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID,
    selector.source_layer_id
  );
}

auto feature_state_needs_warmup(
  const FeatureStateSnapshot& desired,
  const std::unordered_set<std::string>& rendered_source_ids,
  const mln::UpdateParameters& update
) -> bool {
  return std::ranges::any_of(*update.sources, [&](const auto& source) {
    if (rendered_source_ids.contains(source->id)) {
      return false;
    }
    const auto layers = desired.sources.find(source->id);
    return layers != desired.sources.end() && !layers->second.empty();
  });
}

void apply_feature_state_diff(
  mln::Renderer& renderer, const mln::UpdateParameters& update,
  const FeatureStateSnapshot& desired, FeatureStateSnapshot& applied,
  const std::unordered_set<std::string>& rendered_source_ids
) {
  const auto sync_layer = [&](
                            const std::string& source_id,
                            const FeatureStateLayerId& layer_id,
                            const FeatureStateFeatures* desired_features,
                            const FeatureStateFeatures* applied_features
                          ) {
    static const FeatureStateFeatures empty{};
    const auto& desired_map =
      desired_features == nullptr ? empty : *desired_features;
    const auto& applied_map =
      applied_features == nullptr ? empty : *applied_features;

    if (desired_features == nullptr) {
      if (applied_features != nullptr) {
        renderer.removeFeatureState(
          source_id, layer_id, std::nullopt, std::nullopt
        );
      }
      return;
    }

    for (const auto& [feature_id, _] : applied_map) {
      if (!desired_map.contains(feature_id)) {
        renderer.removeFeatureState(
          source_id, layer_id, feature_id, std::nullopt
        );
      }
    }

    for (const auto& [feature_id, desired_state] : desired_map) {
      const auto applied_it = applied_map.find(feature_id);
      const mln::FeatureState* applied_state =
        applied_it == applied_map.end() ? nullptr : &applied_it->second;
      if (applied_state != nullptr) {
        for (const auto& [key, _] : *applied_state) {
          if (!desired_state.contains(key)) {
            renderer.removeFeatureState(source_id, layer_id, feature_id, key);
          }
        }
      }
      auto delta = mln::FeatureState{};
      for (const auto& [key, value] : desired_state) {
        if (
          applied_state == nullptr || !applied_state->contains(key) ||
          !((*applied_state).at(key) == value)
        ) {
          delta[key] = value;
        }
      }
      if (!delta.empty()) {
        renderer.setFeatureState(source_id, layer_id, feature_id, delta);
      }
    }
  };

  for (const auto& source : *update.sources) {
    const auto source_was_rendered = rendered_source_ids.contains(source->id);
    const auto* desired_layers = layer_or_empty(desired, source->id, true);
    const auto* applied_layers =
      layer_or_empty(applied, source->id, source_was_rendered);

    if (desired_layers == nullptr && applied_layers == nullptr) {
      applied.sources.erase(source->id);
      continue;
    }

    static const FeatureStateLayers empty_layers{};
    const auto& desired_map =
      desired_layers == nullptr ? empty_layers : *desired_layers;
    const auto& applied_map =
      applied_layers == nullptr ? empty_layers : *applied_layers;

    for (const auto& [layer_id, features] : applied_map) {
      if (!desired_map.contains(layer_id)) {
        sync_layer(source->id, layer_id, nullptr, &features);
      }
    }
    for (const auto& [layer_id, features] : desired_map) {
      const auto applied_it = applied_map.find(layer_id);
      sync_layer(
        source->id, layer_id, &features,
        applied_it == applied_map.end() ? nullptr : &applied_it->second
      );
    }

    if (desired_layers == nullptr) {
      applied.sources.erase(source->id);
    } else {
      applied.sources[source->id] = *desired_layers;
    }
  }

  std::erase_if(applied.sources, [&](const auto& entry) {
    return !update_has_source(update, entry.first);
  });
}

void remember_rendered_sources(
  std::unordered_set<std::string>& rendered_source_ids,
  const mln::UpdateParameters& update
) {
  std::erase_if(rendered_source_ids, [&](const auto& source_id) {
    return !update_has_source(update, source_id);
  });
  for (const auto& source : *update.sources) {
    rendered_source_ids.insert(source->id);
  }
}

}  // namespace mln::core
