// Ports maplibre-native-ffi src/c_api/query.cpp, src/render/render_session_query.cpp
// and the queries of src/render/render_session_common.cpp:
// mln_render_session_query_rendered_features,
// mln_render_session_query_source_features and
// mln_render_session_query_feature_extensions, for the offscreen and Metal
// surface sessions, whose renderers live on the runtime thread.
//
// A call validates and copies its input on the caller's thread and queries
// the session's renderer on the runtime thread, in order with its frames.
//
// Deviations, from the Rust core:
// - Its rendered features don't name their layer, so a query that finds
//   features in several layers queries each of those layers once more to
//   name them. The location indicator's hits (its DynamicFeatureIndex) are
//   not ported, so a query doesn't find them.
// - Renderer::queryFeatureExtensions is not ported; the supercluster
//   extension reads the GeoJSON source of the map's latest update, as
//   RenderGeoJSONSource does. A cluster ID that names no cluster answers an
//   empty feature collection (or expansion zoom 0), where C++ throws.

use crate::delivery::{Completion, Value};
use crate::diagnostics::{Failure, boundary};
use crate::handles;
use crate::map::{self, MapEntry};
use crate::session::SessionShared;
use crate::types::*;
use maplibre_native_next::geojson::{self, GeoJSON};
use maplibre_native_next::maplibre_native_base::{Feature as GeoJsonFeature, Value as JsonValue};
use maplibre_native_next::mln_conversion::{Convertible, Error as ConversionError};
use maplibre_native_next::mln_gfx::{BackendScope, RendererBackend, ScopeType};
use maplibre_native_next::mln_renderer::query::{RenderedQueryOptions, SourceQueryOptions};
use maplibre_native_next::mln_renderer::{Renderer, UpdateParameters};
use maplibre_native_next::mln_style::conversion::convert;
use maplibre_native_next::mln_style::filter::Filter;
use maplibre_native_next::mln_style::sources::geojson_source_impl;
use maplibre_native_next::mln_util::feature::Feature;
use maplibre_native_next::mln_util::geo::ScreenCoordinate;
use std::collections::{BTreeMap, BTreeSet};
use std::rc::Rc;
use std::sync::Arc;

/// One `mln_rendered_feature`'s text.
pub struct RenderedFeature {
    feature: String,
    layer_id: String,
    source_id: String,
    source_layer_id: String,
    state: String,
}

impl RenderedFeature {
    /// The feature, layer, source, source layer and state texts.
    pub fn into_texts(self) -> [LentText; 5] {
        [
            LentText::new(self.feature),
            LentText::new(self.layer_id),
            LentText::new(self.source_id),
            LentText::new(self.source_layer_id),
            LentText::new(self.state),
        ]
    }
}

/// One `mln_source_feature`'s text.
pub struct SourceFeature {
    feature: String,
    source_layer_id: String,
}

impl SourceFeature {
    /// The feature and source layer texts, then three empty ones.
    pub fn into_texts(self) -> [LentText; 5] {
        [
            LentText::new(self.feature),
            LentText::new(self.source_layer_id),
            LentText::new(""),
            LentText::new(""),
            LentText::new(""),
        ]
    }
}

/// Runs `body` with a session's renderer and its backend, on the runtime
/// thread: an offscreen session's or a Metal surface session's. `None` when
/// the session has none.
pub(crate) fn with_session_renderer<R>(
    session: u64,
    body: impl FnOnce(&mut Renderer, &dyn RendererBackend) -> R,
) -> Option<R> {
    if let Some(entry) = crate::session::session_entry(session) {
        let backend = entry.frontend.get_backend();
        return Some(
            entry
                .frontend
                .with_renderer(|renderer| body(renderer, backend)),
        );
    }
    crate::metal_surface::with_renderer(session, body)
}

/// Submits `body` as driver work of `session` (C++
/// `enqueue_driver_result_operation`): it runs on the runtime thread, in order
/// with the session's frames, and completes with its value.
///
/// # Safety
/// `completion` as for `Completion::required`.
pub(crate) unsafe fn submit(
    session: mln_render_session,
    completion: *const mln_completion,
    body: impl FnOnce(u64, &Rc<MapEntry>, &Arc<SessionShared>) -> Result<Value, Failure>
    + Send
    + 'static,
) -> Result<(), Failure> {
    // SAFETY: the caller's contract.
    let completion = unsafe { Completion::required(completion) }?;
    let shared = handles::session(session)?;
    if shared.state() != MLN_RENDER_SESSION_STATE_ATTACHED {
        return Err(Failure::invalid_state("render session is not attached"));
    }
    if shared.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD {
        return Err(Failure::new(
            MLN_STATUS_UNSUPPORTED,
            "a WebGPU surface session runs no driver work on its runtime yet",
        ));
    }
    let session = session.0;
    shared.map.runtime.clone().post(move || {
        let result = match map::entry(shared.map.handle) {
            Some(map) => body(session, &map, &shared),
            None => Err(Failure::invalid_state("render session is detached")),
        };
        match result {
            Ok(value) => completion.ok_with(0, value),
            Err(failure) => completion.finish(failure.status, 0, failure.message, Value::None),
        }
    });
    Ok(())
}

fn detached() -> Failure {
    Failure::invalid_state("render session is detached")
}

/// `copy_view`.
fn copy_view(view: mln_buffer_view) -> Result<String, Failure> {
    // SAFETY: C API contract for buffer views.
    match unsafe { view.bytes() } {
        Some(bytes) => Ok(String::from_utf8_lossy(bytes).into_owned()),
        None => Err(Failure::invalid_argument(
            "buffer view data must not be null when size is nonzero",
        )),
    }
}

/// `validate_bytes`.
fn validate_bytes(view: mln_buffer_view, name: &str) -> Result<&'static [u8], Failure> {
    if view.size == 0 {
        return Err(Failure::invalid_argument(format!(
            "{name} must not be empty"
        )));
    }
    if view.data.is_null() {
        return Err(Failure::invalid_argument(format!(
            "{name} data must not be null"
        )));
    }
    // SAFETY: C API contract for buffer views; the bytes outlive the call
    // that validates them, which is all that reads them.
    let bytes: &'static [u8] =
        unsafe { std::slice::from_raw_parts(view.data.cast::<u8>(), view.size) };
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument(format!(
            "{name} must not contain embedded NUL"
        )));
    }
    Ok(bytes)
}

/// `convertJSON<Filter>`.
fn parse_filter(json: &str) -> Result<Filter, String> {
    let value: serde_json::Value = serde_json::from_str(json).map_err(|error| error.to_string())?;
    let mut error = ConversionError::default();
    convert::<Filter>(&Convertible::new(&value), &mut error).ok_or(error.message)
}

/// `copy_filter`: the filter's JSON once it parses. An empty filter is none.
fn copy_filter(filter: mln_buffer_view) -> Result<Option<String>, Failure> {
    let text = copy_view(filter)?;
    if text.is_empty() {
        return Ok(None);
    }
    validate_bytes(filter, "style filter")?;
    parse_filter(&text)
        .map_err(|message| Failure::invalid_argument(format!("style filter: {message}")))?;
    Ok(Some(text))
}

/// The filter that `copy_filter` checked, on the runtime thread.
fn filter_of(json: &Option<String>) -> Result<Option<Filter>, Failure> {
    json.as_deref()
        .map(parse_filter)
        .transpose()
        .map_err(|message| Failure::invalid_argument(format!("style filter: {message}")))
}

/// Copies `count` IDs at `ids`.
///
/// # Safety
/// `ids` points at `count` readable buffer views, or `count` is 0.
unsafe fn copy_ids(ids: *const mln_buffer_view, count: usize) -> Result<Vec<String>, Failure> {
    if count == 0 {
        return Ok(Vec::new());
    }
    // SAFETY: the caller's contract.
    unsafe { std::slice::from_raw_parts(ids, count) }
        .iter()
        .map(|id| copy_view(*id))
        .collect()
}

/// `validate_string_views` on the runtime thread.
fn validate_ids(ids: &Option<Vec<String>>, name: &str) -> Result<(), Failure> {
    if ids
        .as_ref()
        .is_some_and(|ids| ids.iter().any(String::is_empty))
    {
        return Err(Failure::invalid_argument(format!(
            "{name} must not contain empty strings"
        )));
    }
    Ok(())
}

fn serialize_feature(feature: &Feature) -> String {
    geojson::stringify(&GeoJSON::Feature(feature.to_geojson_feature()))
}

fn finite(point: mln_screen_point) -> Result<ScreenCoordinate, Failure> {
    if !point.x.is_finite() || !point.y.is_finite() {
        return Err(Failure::invalid_argument(
            "screen point coordinates must be finite",
        ));
    }
    Ok(ScreenCoordinate::new(point.x, point.y))
}

/// The shape of a rendered query.
enum Geometry {
    Point(ScreenCoordinate),
    Box(mln_screen_box),
    LineString(Vec<ScreenCoordinate>),
}

/// `clip_screen_box_to_viewport`: a box that overcovers the viewport finds
/// nothing in the core's tile space, so the query covers the viewport's part
/// of it.
fn clip_box(
    b: mln_screen_box,
    width: u32,
    height: u32,
) -> Option<(ScreenCoordinate, ScreenCoordinate)> {
    let (width, height) = (f64::from(width), f64::from(height));
    let (min_x, max_x) = (b.min.x.min(b.max.x), b.min.x.max(b.max.x));
    let (min_y, max_y) = (b.min.y.min(b.max.y), b.min.y.max(b.max.y));
    if min_x > width || min_y > height || max_x < 0.0 || max_y < 0.0 {
        return None;
    }
    Some((
        ScreenCoordinate::new(min_x.max(0.0), min_y.max(0.0)),
        ScreenCoordinate::new(max_x.min(width), max_y.min(height)),
    ))
}

/// The query's polygon, as Renderer::queryRenderedFeatures builds it.
fn polygon_of(geometry: &Geometry, width: u32, height: u32) -> Option<Vec<ScreenCoordinate>> {
    match geometry {
        Geometry::Point(point) => Some(vec![*point]),
        Geometry::Box(b) => {
            let (min, max) = clip_box(*b, width, height)?;
            Some(vec![
                min,
                ScreenCoordinate::new(max.x, min.y),
                max,
                ScreenCoordinate::new(min.x, max.y),
                min,
            ])
        }
        Geometry::LineString(points) => Some(points.clone()),
    }
}

/// Queries the renderer and names the layer of each hit: the core's
/// features don't carry it, so when the hits may come from several layers,
/// each candidate layer is queried on its own, from the topmost down, the
/// order of the core's results.
fn query_rendered(
    renderer: &Renderer,
    update: Option<&UpdateParameters>,
    polygon: &[ScreenCoordinate],
    layer_ids: Option<Vec<String>>,
    filter: Option<Filter>,
) -> Vec<(String, Feature)> {
    let options = RenderedQueryOptions::new(layer_ids.clone(), filter.clone());
    let features = renderer.query_rendered_features(polygon, &options);
    if features.is_empty() {
        return Vec::new();
    }
    // The style's layers from the topmost, with their sources.
    let mut layers: Vec<(String, String)> = update
        .map(|update| {
            update
                .layers
                .iter()
                .rev()
                .map(|layer| (layer.base().id.clone(), layer.base().source.clone()))
                .collect()
        })
        .unwrap_or_default();
    if let Some(ids) = &layer_ids {
        let ids: BTreeSet<&String> = ids.iter().collect();
        layers.retain(|(id, _)| ids.contains(id));
    }
    let sources: BTreeSet<&str> = features
        .iter()
        .map(|feature| feature.source.as_str())
        .collect();
    layers.retain(|(_, source)| sources.contains(source.as_str()) || source.is_empty());
    if let [(id, _)] = layers.as_slice() {
        return features
            .into_iter()
            .map(|feature| (id.clone(), feature))
            .collect();
    }
    let mut named = Vec::with_capacity(features.len());
    for (id, _) in layers {
        let options = RenderedQueryOptions::new(Some(vec![id.clone()]), filter.clone());
        named.extend(
            renderer
                .query_rendered_features(polygon, &options)
                .into_iter()
                .map(|feature| (id.clone(), feature)),
        );
    }
    named
}

/// # Safety
/// C API contract (query.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_query_rendered_features(
    session: mln_render_session,
    geometry: *const mln_rendered_query_geometry,
    options: *const mln_rendered_feature_query_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(geometry) = read_sized(geometry) else {
                return Err(Failure::invalid_argument(
                    "rendered query geometry is null or too small",
                ));
            };
            let geometry = match geometry.type_ {
                MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT => {
                    Geometry::Point(finite(geometry.data.point)?)
                }
                MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX => {
                    let b = geometry.data.box_;
                    finite(b.min)?;
                    finite(b.max)?;
                    Geometry::Box(b)
                }
                MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING => {
                    let line = geometry.data.line_string;
                    if line.point_count == 0 {
                        return Err(Failure::invalid_argument(
                            "query line string must contain points",
                        ));
                    }
                    if line.points.is_null() {
                        return Err(Failure::invalid_argument(
                            "line string points must not be null",
                        ));
                    }
                    let points = std::slice::from_raw_parts(line.points, line.point_count);
                    Geometry::LineString(
                        points
                            .iter()
                            .map(|point| finite(*point))
                            .collect::<Result<_, _>>()?,
                    )
                }
                _ => {
                    return Err(Failure::invalid_argument(
                        "rendered query geometry type is invalid",
                    ));
                }
            };
            let (layer_ids, filter) = if options.is_null() {
                (None, None)
            } else {
                let Some(options) = read_sized(options) else {
                    return Err(Failure::invalid_argument(
                        "rendered feature query options size is too small",
                    ));
                };
                if options.fields & !MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS != 0 {
                    return Err(Failure::invalid_argument(
                        "rendered feature query options have unknown fields",
                    ));
                }
                let has_layer_ids =
                    options.fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS != 0;
                if has_layer_ids && options.layer_id_count != 0 && options.layer_ids.is_null() {
                    return Err(Failure::invalid_argument(
                        "layer_ids must not be null when layer_id_count is nonzero",
                    ));
                }
                let layer_ids = if has_layer_ids {
                    Some(copy_ids(options.layer_ids, options.layer_id_count)?)
                } else {
                    None
                };
                (layer_ids, copy_filter(options.filter)?)
            };
            submit(session, completion, move |session, map, shared| {
                validate_ids(&layer_ids, "query layer IDs")?;
                let filter = filter_of(&filter)?;
                let extent = shared.state.lock().unwrap().extent;
                let update = map.frontend.last_update();
                let features = with_session_renderer(session, |renderer, backend| {
                    let _scope = BackendScope::new(backend, ScopeType::Explicit);
                    match polygon_of(&geometry, extent.width, extent.height) {
                        Some(polygon) => {
                            query_rendered(renderer, update.as_deref(), &polygon, layer_ids, filter)
                        }
                        None => Vec::new(),
                    }
                })
                .ok_or_else(detached)?;
                Ok(Value::RenderedFeatures(
                    features
                        .into_iter()
                        .map(|(layer_id, feature)| RenderedFeature {
                            feature: serialize_feature(&feature),
                            layer_id,
                            source_id: feature.source.clone(),
                            source_layer_id: feature.source_layer.clone(),
                            state: if feature.state.is_empty() {
                                String::new()
                            } else {
                                crate::feature_state::serialize(&JsonValue::Object(
                                    feature.state.clone(),
                                ))
                            },
                        })
                        .collect(),
                ))
            })
        })
    }
}

/// # Safety
/// C API contract (query.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_query_source_features(
    session: mln_render_session,
    source_id: mln_buffer_view,
    options: *const mln_source_feature_query_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let source_id = copy_view(source_id)?;
            if source_id.is_empty() {
                return Err(Failure::invalid_argument("source_id must not be empty"));
            }
            let (source_layer_ids, filter) = if options.is_null() {
                (None, None)
            } else {
                let Some(options) = read_sized(options) else {
                    return Err(Failure::invalid_argument(
                        "source feature query options size is too small",
                    ));
                };
                if options.fields & !MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS != 0 {
                    return Err(Failure::invalid_argument(
                        "source feature query options have unknown fields",
                    ));
                }
                let has_layer_ids =
                    options.fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS != 0;
                if has_layer_ids
                    && options.source_layer_id_count != 0
                    && options.source_layer_ids.is_null()
                {
                    return Err(Failure::invalid_argument(
                        "source_layer_ids must not be null when source_layer_id_count is nonzero",
                    ));
                }
                let layer_ids = if has_layer_ids {
                    Some(copy_ids(
                        options.source_layer_ids,
                        options.source_layer_id_count,
                    )?)
                } else {
                    None
                };
                (layer_ids, copy_filter(options.filter)?)
            };
            submit(session, completion, move |session, _, _| {
                validate_ids(&source_layer_ids, "query source layer IDs")?;
                let filter = filter_of(&filter)?;
                let features = with_session_renderer(session, |renderer, backend| {
                    let _scope = BackendScope::new(backend, ScopeType::Explicit);
                    renderer.query_source_features(
                        &source_id,
                        &SourceQueryOptions::new(source_layer_ids, filter),
                    )
                })
                .ok_or_else(detached)?;
                Ok(Value::SourceFeatures(
                    features
                        .iter()
                        .map(|feature| SourceFeature {
                            feature: serialize_feature(feature),
                            source_layer_id: feature.source_layer.clone(),
                        })
                        .collect(),
                ))
            })
        })
    }
}

/// `validate_non_empty_string`.
fn non_empty(view: mln_buffer_view, name: &str) -> Result<String, Failure> {
    // SAFETY: C API contract for buffer views.
    let Some(bytes) = (unsafe { view.bytes() }) else {
        return Err(Failure::invalid_argument("string data must not be null"));
    };
    if bytes.is_empty() {
        return Err(Failure::invalid_argument(format!(
            "{name} must not be empty"
        )));
    }
    Ok(String::from_utf8_lossy(bytes).into_owned())
}

/// `parse_json_document`.
fn parse_json(view: mln_buffer_view, name: &str) -> Result<serde_json::Value, Failure> {
    let bytes = validate_bytes(view, name)?;
    serde_json::from_slice(bytes)
        .map_err(|error| Failure::invalid_argument(format!("{name} is not valid JSON: {error}")))
}

/// `to_native_feature`.
fn feature_of(view: mln_buffer_view) -> Result<GeoJsonFeature, Failure> {
    let json = parse_json(view, "feature")?;
    match geojson::convert(&json) {
        Ok(GeoJSON::Feature(feature)) => Ok(feature),
        Ok(_) => Err(Failure::invalid_argument(
            "feature must contain one GeoJSON Feature",
        )),
        Err(error) => Err(Failure::invalid_argument(format!(
            "feature is invalid: {error}"
        ))),
    }
}

/// `to_feature_extension_arguments`: `None` for no arguments.
fn arguments_of(view: mln_buffer_view) -> Result<Option<BTreeMap<String, JsonValue>>, Failure> {
    if view.size == 0 {
        return Ok(None);
    }
    let json = parse_json(view, "JSON value")?;
    match geojson::convert_value(&json) {
        JsonValue::Object(object) => Ok(Some(object)),
        _ => Err(Failure::invalid_argument(
            "feature extension arguments must be a JSON object",
        )),
    }
}

/// `RenderGeoJSONSource::queryFeatureExtensions` on the GeoJSON source of
/// `update`: the supercluster extension's answer as JSON, `null` when the
/// source has none.
fn feature_extension(
    update: Option<&UpdateParameters>,
    source_id: &str,
    feature: &GeoJsonFeature,
    extension: &str,
    field: &str,
    arguments: Option<&BTreeMap<String, JsonValue>>,
) -> String {
    const NULL: &str = "null";
    if extension != "supercluster" || !matches!(field, "children" | "leaves" | "expansion-zoom") {
        return NULL.to_owned();
    }
    let Some(JsonValue::Uint(cluster_id)) = feature.properties.get("cluster_id") else {
        return NULL.to_owned();
    };
    let cluster_id = *cluster_id as u32;
    let data = update
        .and_then(|update| {
            update
                .sources
                .iter()
                .find(|source| source.base().id == source_id)
                .cloned()
        })
        .and_then(|source| {
            source
                .as_any()
                .downcast_ref::<geojson_source_impl::Impl>()
                .and_then(|source| source.get_data()?.upgrade())
        });
    let Some(data) = data else {
        return NULL.to_owned();
    };
    let uint = |name: &str| match arguments.and_then(|arguments| arguments.get(name)) {
        Some(JsonValue::Uint(value)) => Some(*value as u32),
        _ => None,
    };
    let collection = match field {
        "children" => data.get_children(cluster_id),
        "leaves" => match uint("limit") {
            // An offset needs a limit.
            Some(limit) => data.get_leaves(cluster_id, limit, uint("offset").unwrap_or(0)),
            None => data.get_leaves(cluster_id, 10, 0),
        },
        _ => {
            return crate::feature_state::serialize(&JsonValue::Uint(u64::from(
                data.get_cluster_expansion_zoom(cluster_id),
            )));
        }
    };
    geojson::stringify(&GeoJSON::FeatureCollection(collection))
}

/// # Safety
/// C API contract (query.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_query_feature_extensions(
    session: mln_render_session,
    source_id: mln_buffer_view,
    feature: mln_buffer_view,
    extension: mln_buffer_view,
    extension_field: mln_buffer_view,
    arguments: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let source_id = non_empty(source_id, "source_id")?;
            let extension = non_empty(extension, "extension")?;
            let field = non_empty(extension_field, "extension_field")?;
            let feature = feature_of(feature)?;
            let arguments = arguments_of(arguments)?;
            // The feature's values hold no Rc, so it may cross threads.
            struct Input(GeoJsonFeature, Option<BTreeMap<String, JsonValue>>);
            // SAFETY: see above; the values are plain data.
            unsafe impl Send for Input {}
            let input = Input(feature, arguments);
            submit(session, completion, move |session, map, _| {
                let input = input;
                crate::metal_surface::is_live(session)
                    .then_some(())
                    .or_else(|| crate::session::session_entry(session).map(|_| ()))
                    .ok_or_else(detached)?;
                let update = map.frontend.last_update();
                let json = feature_extension(
                    update.as_deref(),
                    &source_id,
                    &input.0,
                    &extension,
                    &field,
                    input.1.as_ref(),
                );
                Ok(Value::BufferView(json.into_bytes()))
            })
        })
    }
}
