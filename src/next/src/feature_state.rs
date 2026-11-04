// Ports maplibre-native-ffi src/map/feature_state.cpp and the feature state
// parts of src/c_api/map.cpp, src/map/map.cpp and render_session_common.cpp:
// mln_map_set_feature_state, mln_map_get_feature_state and
// mln_map_remove_feature_state.
//
// The map owns the state (`Store`, in its MapEntry on the runtime thread), so
// it needs no style, source or render session. Before a session renders an
// update, it pushes what changed since its last frame into its renderer
// (`Store::push`). A renderer keeps the state of a source only once it has
// the source, so when an update brings a source that has state, the session
// first renders the update once without showing it (`Store::needs_warmup`).
//
// Deviation: C++ pushes the state that the map published with the update a
// session renders, which keeps a change inside a command group out of the
// frames until the group ends; the port pushes the store's current state.

use crate::delivery::Value;
use crate::diagnostics::{Failure, boundary};
use crate::map::command;
use crate::types::*;
use maplibre_native_next::maplibre_native_base::Value as JsonValue;
use maplibre_native_next::mln_renderer::{Renderer, UpdateParameters};
use maplibre_native_next::mln_style::collection::ImmutableVector;
use maplibre_native_next::mln_style::conversion::stringify::{Writer, stringify};
use maplibre_native_next::mln_style::expression::expression::FeatureState;
use maplibre_native_next::mln_style::source_impl;
use std::cell::RefCell;
use std::collections::{BTreeMap, HashSet};
use std::rc::Rc;
use std::sync::Arc;

/// A source layer ID, `None` for a source without source layers.
type LayerId = Option<String>;
type Features = BTreeMap<String, FeatureState>;
type Layers = BTreeMap<LayerId, Features>;

/// `FeatureStateSnapshot`: each source's layers' features' state.
#[derive(Clone, Default)]
pub struct Snapshot {
    sources: BTreeMap<String, Layers>,
}

/// `FeatureStateStore`: the map's feature state. Each change publishes a new
/// snapshot, so a session tells a change from the snapshot it last pushed.
#[derive(Default)]
pub struct Store {
    published: RefCell<Rc<Snapshot>>,
}

/// What a session's renderer holds of the store: C++ the session's
/// `rendered_sources`, `rendered_source_ids`, `applied_feature_state` and
/// `pushed_feature_state`.
#[derive(Default)]
pub struct Applied {
    rendered_sources: Option<ImmutableVector<dyn source_impl::Impl>>,
    rendered_source_ids: HashSet<String>,
    applied: Snapshot,
    pushed: Option<Rc<Snapshot>>,
}

fn prune(
    sources: &mut BTreeMap<String, Layers>,
    source_id: &str,
    layer_id: &LayerId,
    feature_id: Option<&str>,
) {
    let Some(layers) = sources.get_mut(source_id) else {
        return;
    };
    let Some(features) = layers.get_mut(layer_id) else {
        return;
    };
    if let Some(feature_id) = feature_id
        && features.get(feature_id).is_some_and(FeatureState::is_empty)
    {
        features.remove(feature_id);
    }
    if features.is_empty() {
        layers.remove(layer_id);
    }
    if layers.is_empty() {
        sources.remove(source_id);
    }
}

impl Store {
    /// The current state.
    pub fn snapshot(&self) -> Rc<Snapshot> {
        self.published.borrow().clone()
    }

    /// Merges `state` into a feature's; whether that changed it.
    pub fn set(
        &self,
        source_id: String,
        layer_id: LayerId,
        feature_id: String,
        state: FeatureState,
    ) -> bool {
        if state.is_empty() {
            return false;
        }
        let mut published = self.published.borrow_mut();
        let current = published
            .sources
            .get(&source_id)
            .and_then(|layers| layers.get(&layer_id)?.get(&feature_id));
        if let Some(current) = current
            && state
                .iter()
                .all(|(key, value)| current.get(key) == Some(value))
        {
            return false;
        }
        let snapshot = Rc::make_mut(&mut published);
        let feature = snapshot
            .sources
            .entry(source_id)
            .or_default()
            .entry(layer_id)
            .or_default()
            .entry(feature_id)
            .or_default();
        for (key, value) in state {
            feature.insert(key, value);
        }
        true
    }

    /// Removes the selected state; whether there was any.
    pub fn remove(
        &self,
        source_id: &str,
        layer_id: &LayerId,
        feature_id: Option<&str>,
        state_key: Option<&str>,
    ) -> bool {
        let mut published = self.published.borrow_mut();
        let Some(features) = published
            .sources
            .get(source_id)
            .and_then(|layers| layers.get(layer_id))
        else {
            return false;
        };
        if let Some(feature_id) = feature_id {
            match features.get(feature_id) {
                None => return false,
                Some(state) if state_key.is_some_and(|key| !state.contains_key(key)) => {
                    return false;
                }
                Some(_) => {}
            }
        }
        let snapshot = Rc::make_mut(&mut published);
        let layers = snapshot.sources.get_mut(source_id).expect("checked above");
        match feature_id {
            None => {
                layers.remove(layer_id);
                if layers.is_empty() {
                    snapshot.sources.remove(source_id);
                }
            }
            Some(feature_id) => {
                let features = layers.get_mut(layer_id).expect("checked above");
                match state_key {
                    Some(key) => {
                        features
                            .get_mut(feature_id)
                            .expect("checked above")
                            .remove(key);
                    }
                    None => {
                        features.remove(feature_id);
                    }
                }
                prune(&mut snapshot.sources, source_id, layer_id, Some(feature_id));
            }
        }
        true
    }

    /// A feature's state, empty when it has none.
    pub fn get(&self, source_id: &str, layer_id: &LayerId, feature_id: &str) -> FeatureState {
        let published = self.published.borrow();
        published
            .sources
            .get(source_id)
            .and_then(|layers| layers.get(layer_id)?.get(feature_id))
            .cloned()
            .unwrap_or_default()
    }

    /// `feature_state_needs_warmup`: whether `update` brings a source to the
    /// renderer that has state, which the renderer takes only once it has
    /// rendered the source.
    pub fn needs_warmup(&self, applied: &Applied, update: &UpdateParameters) -> bool {
        if !sources_changed(applied, update) {
            return false;
        }
        let desired = self.snapshot();
        update.sources.iter().any(|source| {
            let id = &source.base().id;
            !applied.rendered_source_ids.contains(id)
                && desired
                    .sources
                    .get(id)
                    .is_some_and(|layers| !layers.is_empty())
        })
    }

    /// Pushes what changed since the renderer's last update into it, before
    /// it renders `update`.
    pub fn push(&self, applied: &mut Applied, renderer: &mut Renderer, update: &UpdateParameters) {
        let desired = self.snapshot();
        let sources_changed = sources_changed(applied, update);
        if sources_changed
            || !applied
                .pushed
                .as_ref()
                .is_some_and(|pushed| Rc::ptr_eq(pushed, &desired))
        {
            apply_diff(
                renderer,
                update,
                &desired,
                &mut applied.applied,
                &applied.rendered_source_ids,
            );
            applied.pushed = Some(desired);
        }
        if sources_changed {
            // `reconcile_feature_state_sources`.
            applied.rendered_source_ids = update
                .sources
                .iter()
                .map(|source| source.base().id.clone())
                .collect();
            let ids = &applied.rendered_source_ids;
            applied.applied.sources.retain(|id, _| ids.contains(id));
            applied.rendered_sources = Some(update.sources.clone());
        }
    }
}

fn sources_changed(applied: &Applied, update: &UpdateParameters) -> bool {
    !applied
        .rendered_sources
        .as_ref()
        .is_some_and(|sources| Arc::ptr_eq(sources, &update.sources))
}

/// `apply_feature_state_diff`.
fn apply_diff(
    renderer: &mut Renderer,
    update: &UpdateParameters,
    desired: &Snapshot,
    applied: &mut Snapshot,
    rendered_source_ids: &HashSet<String>,
) {
    let empty_features = Features::new();
    let mut sync_layer = |source_id: &str,
                          layer_id: &LayerId,
                          desired: Option<&Features>,
                          applied: Option<&Features>| {
        let layer = layer_id.as_deref();
        let Some(desired) = desired else {
            if applied.is_some() {
                renderer.remove_feature_state(source_id, layer, None, None);
            }
            return;
        };
        let applied = applied.unwrap_or(&empty_features);
        for feature_id in applied.keys() {
            if !desired.contains_key(feature_id) {
                renderer.remove_feature_state(source_id, layer, Some(feature_id), None);
            }
        }
        for (feature_id, desired_state) in desired {
            let applied_state = applied.get(feature_id);
            if let Some(applied_state) = applied_state {
                for key in applied_state.keys() {
                    if !desired_state.contains_key(key) {
                        renderer.remove_feature_state(
                            source_id,
                            layer,
                            Some(feature_id),
                            Some(key),
                        );
                    }
                }
            }
            let delta: FeatureState = desired_state
                .iter()
                .filter(|(key, value)| {
                    applied_state.and_then(|state| state.get(*key)) != Some(*value)
                })
                .map(|(key, value)| (key.clone(), value.clone()))
                .collect();
            if !delta.is_empty() {
                renderer.set_feature_state(source_id, layer, feature_id, &delta);
            }
        }
    };

    let empty_layers = Layers::new();
    for source in update.sources.iter() {
        let id = &source.base().id;
        let desired_layers = desired.sources.get(id);
        let applied_layers = if rendered_source_ids.contains(id) {
            applied.sources.get(id).cloned()
        } else {
            None
        };
        if desired_layers.is_none() && applied_layers.is_none() {
            applied.sources.remove(id);
            continue;
        }
        let desired_map = desired_layers.unwrap_or(&empty_layers);
        let applied_map = applied_layers.as_ref().unwrap_or(&empty_layers);
        for (layer_id, features) in applied_map {
            if !desired_map.contains_key(layer_id) {
                sync_layer(id, layer_id, None, Some(features));
            }
        }
        for (layer_id, features) in desired_map {
            sync_layer(id, layer_id, Some(features), applied_map.get(layer_id));
        }
        match desired_layers {
            None => {
                applied.sources.remove(id);
            }
            Some(layers) => {
                applied.sources.insert(id.clone(), layers.clone());
            }
        }
    }
}

// MARK: - C API

/// A selector's strings, copied for the runtime thread.
struct Selector {
    source_id: String,
    layer_id: LayerId,
    feature_id: Option<String>,
    state_key: Option<String>,
}

/// `validate_string_view`.
fn text(view: mln_buffer_view) -> Result<String, Failure> {
    // SAFETY: C API contract for buffer views.
    match unsafe { view.bytes() } {
        Some(bytes) => Ok(String::from_utf8_lossy(bytes).into_owned()),
        None => Err(Failure::invalid_argument("string data must not be null")),
    }
}

/// `validate_feature_state_selector` and the selector's strings.
///
/// # Safety
/// `selector` is null or points at a readable `mln_feature_state_selector`
/// whose views name readable bytes.
unsafe fn selector_of(
    selector: *const mln_feature_state_selector,
    require_feature_id: bool,
) -> Result<Selector, Failure> {
    if selector.is_null() {
        return Err(Failure::invalid_argument(
            "feature state selector must not be null",
        ));
    }
    // SAFETY: the caller's contract.
    let Some(selector) = (unsafe { read_sized(selector) }) else {
        return Err(Failure::invalid_argument(
            "mln_feature_state_selector.size is too small",
        ));
    };
    if selector.fields & !MLN_FEATURE_STATE_SELECTOR_FEATURE_ID != 0 {
        return Err(Failure::invalid_argument(
            "feature state selector has unknown fields",
        ));
    }
    let source_id = text(selector.source_id)?;
    if source_id.is_empty() {
        return Err(Failure::invalid_argument(
            "feature state source_id must not be empty",
        ));
    }
    let layer_id = text(selector.source_layer_id)?;
    let has_feature_id = selector.fields & MLN_FEATURE_STATE_SELECTOR_FEATURE_ID != 0;
    let feature_id = if has_feature_id {
        Some(text(selector.feature_id)?)
    } else {
        None
    };
    let state_key = text(selector.state_key)?;
    if require_feature_id && !has_feature_id {
        return Err(Failure::invalid_argument(
            "feature state selector requires feature_id",
        ));
    }
    if !state_key.is_empty() && !has_feature_id {
        return Err(Failure::invalid_argument(
            "feature state selector state_key requires feature_id",
        ));
    }
    Ok(Selector {
        source_id,
        // An empty source layer ID or state key is none.
        layer_id: (!layer_id.is_empty()).then_some(layer_id),
        feature_id,
        state_key: (!state_key.is_empty()).then_some(state_key),
    })
}

/// `serialize_json_value`.
pub fn serialize(value: &JsonValue) -> String {
    let mut writer = Writer::new();
    stringify(&mut writer, value);
    writer.into_string()
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_feature_state(
    map: mln_map,
    selector: *const mln_feature_state_selector,
    state: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let selector = selector_of(selector, true)?;
            if state.data.is_null() || state.size == 0 {
                return Err(Failure::invalid_argument("state must not be empty"));
            }
            let bytes = state.bytes().unwrap_or_default();
            let parsed = serde_json::from_slice::<serde_json::Value>(bytes)
                .ok()
                .map(|json| maplibre_native_next::geojson::convert_value(&json));
            let Some(JsonValue::Object(state)) = parsed else {
                return Err(Failure::invalid_argument(
                    "feature state value must be a JSON object",
                ));
            };
            command(map, completion, true, move |entry| {
                let Selector {
                    source_id,
                    layer_id,
                    feature_id,
                    ..
                } = selector;
                let feature_id = feature_id.expect("set requires feature_id");
                if entry
                    .feature_state
                    .set(source_id, layer_id, feature_id, state)
                {
                    entry.map.borrow_mut().trigger_repaint();
                }
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_feature_state(
    map: mln_map,
    selector: *const mln_feature_state_selector,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let selector = selector_of(selector, true)?;
            command(map, completion, false, move |entry| {
                let feature_id = selector
                    .feature_id
                    .as_deref()
                    .expect("get requires feature_id");
                let state =
                    entry
                        .feature_state
                        .get(&selector.source_id, &selector.layer_id, feature_id);
                Ok(Value::BufferView(
                    serialize(&JsonValue::Object(state)).into_bytes(),
                ))
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_remove_feature_state(
    map: mln_map,
    selector: *const mln_feature_state_selector,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let selector = selector_of(selector, false)?;
            command(map, completion, true, move |entry| {
                if entry.feature_state.remove(
                    &selector.source_id,
                    &selector.layer_id,
                    selector.feature_id.as_deref(),
                    selector.state_key.as_deref(),
                ) {
                    entry.map.borrow_mut().trigger_repaint();
                }
                Ok(Value::None)
            })
        })
    }
}
