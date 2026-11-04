// Ports the layer, light, transition and global state parts of
// maplibre-native-ffi src/c_api/style.cpp and src/map/style.cpp
// (map_add_style_layer_json ... map_get_layer_property) onto
// `mln_style::Style`, with the JSON helpers they use from
// src/geojson/geojson.cpp (validate_bytes, parse_json_document,
// to_native_json_value, serialize_json_value) and src/style/style_value.cpp
// (set_style_conversion_error).
//
// Values cross the API as JSON: an input parses into a `serde_json::Value`,
// which the core's `Convertible` reads as C++ reads a rapidjson document; an
// output is a core `Value` written by the core's port of rapidjson's writer,
// so numbers read back as C++ writes them (`4.0`).

use crate::delivery::{LentValue, Value};
use crate::diagnostics::{Failure, boundary};
use crate::map::command;
use crate::types::*;
use maplibre_native_next::maplibre_native_base::Value as CoreValue;
use maplibre_native_next::mln_conversion::{Convertible, Error as ConversionError};
use maplibre_native_next::mln_style::conversion::stringify::{Writer, stringify};
use maplibre_native_next::mln_style::conversion::{Converter, convert_layer};
use maplibre_native_next::mln_style::filter::Filter;
use maplibre_native_next::mln_style::layer::{Layer, layer_type_info};
use maplibre_native_next::mln_style::light::Light;
use maplibre_native_next::mln_style::style_property::Kind as PropertyKind;
use maplibre_native_next::mln_style::transition_options::TransitionOptions;
use maplibre_native_next::mln_style_types::VisibilityType;
use maplibre_native_next::mln_util::chrono::Duration;
use std::ffi::c_void;

// MARK: - Inputs (c_api/style.cpp)

/// The bytes of a view, which may be empty (C++ `valid_view` + `OwnedView`).
fn view_bytes(view: mln_buffer_view, name: &str) -> Result<Vec<u8>, Failure> {
    // SAFETY: C API contract for buffer views.
    match unsafe { view.bytes() } {
        Some(bytes) => Ok(bytes.to_vec()),
        None => Err(Failure::invalid_argument(format!("{name} is invalid"))),
    }
}

/// The bytes of a view that must not be empty (C++ `valid_non_empty_view`).
fn non_empty_view_bytes(view: mln_buffer_view, name: &str) -> Result<Vec<u8>, Failure> {
    if view.size == 0 {
        return Err(Failure::invalid_argument(format!("{name} must not be empty")));
    }
    view_bytes(view, name)
}

/// An ID or name, which the core takes as a `&str`.
fn text(bytes: Vec<u8>, name: &str) -> Result<String, Failure> {
    String::from_utf8(bytes).map_err(|_| Failure::invalid_argument(format!("{name} must be UTF-8")))
}

// MARK: - JSON (geojson/geojson.cpp, style/style_value.cpp)

/// C++ `validate_bytes`: the checks before a parse.
pub(crate) fn validate_bytes(bytes: Option<&[u8]>, size: usize, name: &str) -> Result<(), Failure> {
    if size == 0 {
        return Err(Failure::invalid_argument(format!("{name} must not be empty")));
    }
    let Some(bytes) = bytes else {
        return Err(Failure::invalid_argument(format!("{name} data must not be null")));
    };
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument(format!("{name} must not contain embedded NUL")));
    }
    Ok(())
}

/// C++ `parse_json_document` over bytes already copied from a view.
fn parse_json(bytes: &[u8], name: &str) -> Result<serde_json::Value, Failure> {
    validate_bytes(Some(bytes), bytes.len(), name)?;
    serde_json::from_slice(bytes).map_err(|e| Failure::invalid_argument(format!("{name} is not valid JSON: {e}")))
}

/// C++ `parse_json_document` over a caller's view.
pub(crate) fn parse_json_view(view: mln_buffer_view, name: &str) -> Result<serde_json::Value, Failure> {
    // SAFETY: C API contract for buffer views.
    let bytes = unsafe { view.bytes() };
    validate_bytes(bytes, view.size, name)?;
    parse_json(bytes.unwrap_or_default(), name)
}

/// C++ `serialize_json_value`.
pub(crate) fn serialize_json(value: &CoreValue) -> String {
    let mut writer = Writer::new();
    stringify(&mut writer, value);
    writer.into_string()
}

/// C++ `set_style_conversion_error`.
fn conversion_failure(context: &str, error: &ConversionError) -> Failure {
    Failure::invalid_argument(format!("{context}: {}", error.message))
}

/// C++ `convertJSON<T>`: a parse error is the conversion's error.
fn convert_json<T>(
    json: &[u8],
    context: &str,
    convert: impl FnOnce(&Convertible<'_>, &mut ConversionError) -> Option<T>,
) -> Result<T, Failure> {
    let document: serde_json::Value =
        serde_json::from_slice(json).map_err(|e| conversion_failure(context, &ConversionError::new(e.to_string())))?;
    let mut error = ConversionError::default();
    convert(&Convertible::from(&document), &mut error).ok_or_else(|| conversion_failure(context, &error))
}

// MARK: - Results

/// One layer's `mln_style_layer_info` (C++ `StyleLayerRecord`). The texts
/// carry a NUL after their bytes, as every text the C++ library lends does.
struct LayerRecord {
    id: LentText,
    type_: LentText,
    source_id: LentText,
    source_layer: LentText,
    min_zoom: f64,
    max_zoom: f64,
    visibility: u32,
}

impl LayerRecord {
    fn of(layer: &dyn Layer) -> LayerRecord {
        LayerRecord {
            id: LentText::new(layer.get_id()),
            type_: LentText::new(layer.get_type_info().type_),
            source_id: LentText::new(layer.get_source_id()),
            source_layer: LentText::new(layer.get_source_layer()),
            min_zoom: f64::from(layer.get_min_zoom()),
            max_zoom: f64::from(layer.get_max_zoom()),
            visibility: if layer.get_visibility() == VisibilityType::None {
                MLN_STYLE_LAYER_VISIBILITY_NONE
            } else {
                MLN_STYLE_LAYER_VISIBILITY_VISIBLE
            },
        }
    }
}

/// `mln_style_layer_info`s and the texts they view.
struct LayerInfos {
    // The heap buffers of these texts, which `infos` views, stay in place
    // when the records move.
    _records: Vec<LayerRecord>,
    infos: Vec<mln_style_layer_info>,
}

// SAFETY: `infos` only points into `_records`, which moves with it.
unsafe impl Send for LayerInfos {}

impl LayerInfos {
    fn new(records: Vec<LayerRecord>) -> LayerInfos {
        let infos = records
            .iter()
            .map(|record| mln_style_layer_info {
                id: mln_buffer_view::of_text(&record.id),
                type_: mln_buffer_view::of_text(&record.type_),
                source_id: mln_buffer_view::of_text(&record.source_id),
                source_layer: mln_buffer_view::of_text(&record.source_layer),
                min_zoom: record.min_zoom,
                max_zoom: record.max_zoom,
                visibility: record.visibility,
            })
            .collect();
        LayerInfos { _records: records, infos }
    }
}

impl LentValue for LayerInfos {
    fn view(&self) -> (*const c_void, usize, usize) {
        (self.infos.as_ptr().cast(), size_of::<mln_style_layer_info>(), self.infos.len())
    }
}

impl LentValue for mln_style_transition_options {
    fn view(&self) -> (*const c_void, usize, usize) {
        ((self as *const Self).cast(), size_of::<mln_style_transition_options>(), 1)
    }
}

/// A JSON result, or no value when it is absent.
fn optional_json(json: Option<String>) -> Value {
    match json {
        Some(json) => Value::BufferView(json.into_bytes()),
        None => Value::None,
    }
}

// MARK: - Durations (map/style.cpp)

/// The largest duration in milliseconds, which no accepted value reaches: the
/// nearest double to `Duration::max()` converts back one tick past it.
fn max_native_duration_ms() -> f64 {
    Duration::max().count() as f64 / 1e6
}

fn is_native_duration_ms(milliseconds: f64) -> bool {
    milliseconds.is_finite() && milliseconds >= 0.0 && milliseconds < max_native_duration_ms()
}

fn duration_from_milliseconds(milliseconds: f64) -> Duration {
    Duration::from_nanos((milliseconds * 1e6) as i64)
}

fn milliseconds_from_duration(duration: Duration) -> f64 {
    duration.count() as f64 / 1e6
}

// MARK: - Layers

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_style_layer_json(
    map: mln_map,
    layer_json: mln_buffer_view,
    before_layer_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let json = non_empty_view_bytes(layer_json, "layer_json")?;
            let before = text(view_bytes(before_layer_id, "before_layer_id")?, "before_layer_id")?;
            command(map, completion, true, move |entry| {
                let mut map = entry.map.borrow_mut();
                let mut style = map.get_style_mut();
                let before = (!before.is_empty()).then_some(before);
                if let Some(before) = &before
                    && style.get_layer(before).is_none()
                {
                    return Err(Failure::new(MLN_STATUS_NOT_FOUND, "before_layer_id does not exist"));
                }
                let layer = convert_json(&json, "style layer", convert_layer)?;
                if style.get_layer(&layer.get_id()).is_some() {
                    return Err(Failure::invalid_argument("layer already exists"));
                }
                if layer.get_type_info().source == layer_type_info::Source::Required {
                    let Some(source) = style.get_source(&layer.get_source_id()) else {
                        return Err(Failure::new(MLN_STATUS_NOT_FOUND, "layer source does not exist"));
                    };
                    if !source.supports_layer_type(layer.get_type_info()) {
                        return Err(Failure::invalid_argument("source does not support layer type"));
                    }
                }
                style.add_layer(layer, before.as_deref()).map_err(|e| Failure::invalid_argument(e.to_string()))?;
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_remove_style_layer(
    map: mln_map,
    layer_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = text(non_empty_view_bytes(layer_id, "layer_id")?, "layer_id")?;
            command(map, completion, true, move |entry| {
                match entry.map.borrow_mut().get_style_mut().remove_layer(&id) {
                    Some(_) => Ok(Value::None),
                    None => Err(Failure::new(MLN_STATUS_NOT_FOUND, "layer does not exist")),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_layer(
    map: mln_map,
    layer_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = text(non_empty_view_bytes(layer_id, "layer_id")?, "layer_id")?;
            command(map, completion, false, move |entry| {
                let map = entry.map.borrow();
                let style = map.get_style();
                Ok(match style.get_layer(&id) {
                    Some(layer) => Value::Lent(Box::new(LayerInfos::new(vec![LayerRecord::of(layer)]))),
                    None => Value::None,
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_list_style_layers(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                let map = entry.map.borrow();
                let records = map.get_style().get_layers().into_iter().map(LayerRecord::of).collect();
                Ok(Value::Lent(Box::new(LayerInfos::new(records))))
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_move_style_layer(
    map: mln_map,
    layer_id: mln_buffer_view,
    before_layer_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = text(non_empty_view_bytes(layer_id, "layer_id")?, "layer_id")?;
            let before = text(view_bytes(before_layer_id, "before_layer_id")?, "before_layer_id")?;
            command(map, completion, true, move |entry| {
                let mut map = entry.map.borrow_mut();
                let mut style = map.get_style_mut();
                if style.get_layer(&id).is_none() {
                    return Err(Failure::new(MLN_STATUS_NOT_FOUND, "layer does not exist"));
                }
                let before = (!before.is_empty()).then_some(before);
                if let Some(before) = &before {
                    if *before == id {
                        return Ok(Value::None);
                    }
                    if style.get_layer(before).is_none() {
                        return Err(Failure::new(MLN_STATUS_NOT_FOUND, "before_layer_id does not exist"));
                    }
                }
                let layer = style.remove_layer(&id).expect("the layer exists");
                style
                    .add_layer(layer, before.as_deref())
                    .map_err(|e| Failure::new(MLN_STATUS_NATIVE_ERROR, e.to_string()))?;
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_layer_json(
    map: mln_map,
    layer_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = text(non_empty_view_bytes(layer_id, "layer_id")?, "layer_id")?;
            command(map, completion, false, move |entry| {
                let map = entry.map.borrow();
                let style = map.get_style();
                Ok(optional_json(style.get_layer(&id).map(|layer| serialize_json(&layer.serialize()))))
            })
        })
    }
}

/// C++ `require_layer_takes_source`: the core would log a warning and ignore
/// the field.
fn require_layer_takes_source(layer: &dyn Layer, field: &str) -> Result<(), Failure> {
    if layer.get_type_info().source == layer_type_info::Source::Required {
        return Ok(());
    }
    Err(Failure::invalid_argument(format!("layer type does not take a {field}; layer id is {}", layer.get_id())))
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_layer_property(
    map: mln_map,
    layer_id: mln_buffer_view,
    property_name: mln_buffer_view,
    value: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = text(non_empty_view_bytes(layer_id, "layer_id")?, "layer_id")?;
            let name = text(non_empty_view_bytes(property_name, "property_name")?, "property_name")?;
            let json = non_empty_view_bytes(value, "value")?;
            command(map, completion, true, move |entry| {
                let document = parse_json(&json, "layer property")?;
                let mut map = entry.map.borrow_mut();
                let mut style = map.get_style_mut();
                let Some(mut layer) = style.get_layer_mut(&id) else {
                    return Err(Failure::new(MLN_STATUS_NOT_FOUND, "layer does not exist"));
                };
                if name == "source" || name == "source-layer" {
                    require_layer_takes_source(&*layer, &name)?;
                }
                if document.is_null() {
                    match name.as_str() {
                        "filter" => {
                            layer.set_filter(&Filter::default());
                            return Ok(Value::None);
                        }
                        "minzoom" => {
                            layer.set_min_zoom(f32::NEG_INFINITY);
                            return Ok(Value::None);
                        }
                        "maxzoom" => {
                            layer.set_max_zoom(f32::INFINITY);
                            return Ok(Value::None);
                        }
                        _ => {}
                    }
                }
                match layer.set_property(&name, &Convertible::from(&document), None) {
                    Some(error) => Err(conversion_failure("layer property", &error)),
                    None => Ok(Value::None),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_layer_property(
    map: mln_map,
    layer_id: mln_buffer_view,
    property_name: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = text(non_empty_view_bytes(layer_id, "layer_id")?, "layer_id")?;
            let name = text(non_empty_view_bytes(property_name, "property_name")?, "property_name")?;
            command(map, completion, false, move |entry| {
                let map = entry.map.borrow();
                let style = map.get_style();
                let Some(layer) = style.get_layer(&id) else {
                    return Err(Failure::new(MLN_STATUS_NOT_FOUND, "layer does not exist"));
                };
                let root = match name.as_str() {
                    "filter" => {
                        let filter = layer.get_filter();
                        filter.is_set().then(|| filter.serialize())
                    }
                    "source" | "source-layer" => {
                        let source = if name == "source" { layer.get_source_id() } else { layer.get_source_layer() };
                        (!source.is_empty()).then_some(CoreValue::String(source))
                    }
                    "minzoom" | "maxzoom" => {
                        let (zoom, unbounded) = if name == "minzoom" {
                            (layer.get_min_zoom(), f32::NEG_INFINITY)
                        } else {
                            (layer.get_max_zoom(), f32::INFINITY)
                        };
                        (zoom != unbounded).then_some(CoreValue::Double(f64::from(zoom)))
                    }
                    _ => {
                        let property = layer.get_property(&name);
                        (property.get_kind() != PropertyKind::Undefined).then(|| property.get_value().clone())
                    }
                };
                Ok(optional_json(root.as_ref().map(serialize_json)))
            })
        })
    }
}

// MARK: - Light

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_light_json(
    map: mln_map,
    light_json: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let json = non_empty_view_bytes(light_json, "light_json")?;
            command(map, completion, true, move |entry| {
                let light = convert_json(&json, "style light", <Light as Converter>::convert)?;
                entry.map.borrow_mut().get_style_mut().set_light(light);
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_light_property(
    map: mln_map,
    property_name: mln_buffer_view,
    value: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let name = text(non_empty_view_bytes(property_name, "property_name")?, "property_name")?;
            let json = non_empty_view_bytes(value, "value")?;
            command(map, completion, true, move |entry| {
                let document = parse_json(&json, "style light property")?;
                let mut map = entry.map.borrow_mut();
                let mut style = map.get_style_mut();
                match style.get_light_mut().set_property(&name, &Convertible::from(&document)) {
                    Some(error) => Err(conversion_failure("style light property", &error)),
                    None => Ok(Value::None),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_light_property(
    map: mln_map,
    property_name: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let name = text(non_empty_view_bytes(property_name, "property_name")?, "property_name")?;
            command(map, completion, false, move |entry| {
                let map = entry.map.borrow();
                let property = map.get_style().get_light().get_property(&name);
                Ok(optional_json(
                    (property.get_kind() != PropertyKind::Undefined).then(|| serialize_json(property.get_value())),
                ))
            })
        })
    }
}

// MARK: - Transitions

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_transition_options(
    map: mln_map,
    options: *const mln_style_transition_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if options.is_null() {
                return Err(Failure::invalid_argument("options must not be null"));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("mln_style_transition_options.size is too small"));
            };
            command(map, completion, true, move |entry| {
                const KNOWN_FIELDS: u32 = MLN_STYLE_TRANSITION_OPTION_DURATION
                    | MLN_STYLE_TRANSITION_OPTION_DELAY
                    | MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
                if options.fields & !KNOWN_FIELDS != 0 {
                    return Err(Failure::invalid_argument("mln_style_transition_options.fields contains unknown bits"));
                }
                // The core's default is already on, so an omitted field
                // leaves it alone.
                let mut native = TransitionOptions::default();
                if options.fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0 {
                    native.enable_placement_transitions = options.enable_placement_transitions;
                }
                if options.fields & MLN_STYLE_TRANSITION_OPTION_DURATION != 0 {
                    if !is_native_duration_ms(options.duration_ms) {
                        return Err(Failure::invalid_argument(
                            "transition duration_ms must fit the native duration range",
                        ));
                    }
                    native.duration = Some(duration_from_milliseconds(options.duration_ms));
                }
                if options.fields & MLN_STYLE_TRANSITION_OPTION_DELAY != 0 {
                    if !is_native_duration_ms(options.delay_ms) {
                        return Err(Failure::invalid_argument(
                            "transition delay_ms must fit the native duration range",
                        ));
                    }
                    native.delay = Some(duration_from_milliseconds(options.delay_ms));
                }
                entry.map.borrow_mut().get_style_mut().set_transition_options(&native);
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_transition_options(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                let native = entry.map.borrow().get_style().get_transition_options();
                // The core always holds the placement flag, so it always
                // reports as present.
                let mut result = mln_style_transition_options {
                    size: size_of::<mln_style_transition_options>() as u32,
                    fields: MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS,
                    duration_ms: 0.0,
                    delay_ms: 0.0,
                    enable_placement_transitions: native.enable_placement_transitions,
                };
                if let Some(duration) = native.duration {
                    result.fields |= MLN_STYLE_TRANSITION_OPTION_DURATION;
                    result.duration_ms = milliseconds_from_duration(duration);
                }
                if let Some(delay) = native.delay {
                    result.fields |= MLN_STYLE_TRANSITION_OPTION_DELAY;
                    result.delay_ms = milliseconds_from_duration(delay);
                }
                Ok(Value::Lent(Box::new(result)))
            })
        })
    }
}

// MARK: - Global state

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_global_state_property(
    map: mln_map,
    property_name: mln_buffer_view,
    value: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let name = text(view_bytes(property_name, "property_name")?, "property_name")?;
            let json = non_empty_view_bytes(value, "value")?;
            command(map, completion, true, move |entry| {
                // C++ `to_native_json_value`.
                let value = maplibre_native_next::geojson::convert_value(&parse_json(&json, "JSON value")?);
                let mut map = entry.map.borrow_mut();
                let mut style = map.get_style_mut();
                if !style.is_loaded() {
                    return Err(Failure::invalid_state("style JSON has not loaded"));
                }
                style.set_global_state_property(&name, &value).map_err(|e| Failure::invalid_state(e.to_string()))?;
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_global_state(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                let state = entry.map.borrow().get_style().get_global_state();
                Ok(Value::BufferView(serialize_json(&CoreValue::Object(state)).into_bytes()))
            })
        })
    }
}
