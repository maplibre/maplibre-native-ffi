// Ports maplibre-native-ffi src/c_api/style_expression.cpp and
// src/style/style_expression.cpp onto `mln_expression`: a style expression
// parsed once and evaluated without a map, with the C++ diagnostics. The
// feature adapter ports the `GeoJSONFeature` of the core's expression.cpp,
// which the core keeps private.

use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::style_layers::{parse_json_view, serialize_json, validate_bytes};
use crate::types::*;
use maplibre_native_next::geojson::{self, GeoJSON};
use maplibre_native_next::maplibre_native_base::{Feature, Identifier, PropertyMap, Value as CoreValue};
use maplibre_native_next::mln_conversion::Convertible;
use maplibre_native_next::mln_expression::{
    EvaluationContext, Expression, ParsingContext, Type, TypeAnnotationOption, is_feature_constant, is_zoom_constant,
    type_::Array, value::to_mbgl_value,
};
use maplibre_native_next::mln_tile::geometry_tile_data::{convert_geometry_to_tile, fixup_polygons};
use maplibre_native_next::mln_tile_id::{CanonicalTileID, GeometryCollection, GeometryTileFeature};
use maplibre_native_next::mln_util::geometry::{FeatureType, to_feature_type};
use std::collections::BTreeSet;
use std::sync::Arc;

/// A parsed expression (C++ `StyleExpressionObject`). Immutable after
/// creation, and evaluation only reads it, so any number of threads may
/// evaluate one expression at once.
pub struct StyleExpression {
    expression: Box<dyn Expression>,
}

/// The live expression that `handle` names, or the C++ handle fault.
fn lease(handle: mln_style_expression) -> Result<Arc<StyleExpression>, Failure> {
    match handles::resolve(handles::Kind::StyleExpression, handle)? {
        Object::StyleExpression(expression) => Ok(expression),
        _ => unreachable!(),
    }
}

fn type_of_kind(kind: u32) -> Option<Type> {
    Some(match kind {
        MLN_STYLE_EXPRESSION_TYPE_VALUE => Type::Value,
        MLN_STYLE_EXPRESSION_TYPE_NUMBER => Type::Number,
        MLN_STYLE_EXPRESSION_TYPE_STRING => Type::String,
        MLN_STYLE_EXPRESSION_TYPE_BOOLEAN => Type::Boolean,
        MLN_STYLE_EXPRESSION_TYPE_COLOR => Type::Color,
        MLN_STYLE_EXPRESSION_TYPE_OBJECT => Type::Object,
        MLN_STYLE_EXPRESSION_TYPE_FORMATTED => Type::Formatted,
        MLN_STYLE_EXPRESSION_TYPE_IMAGE => Type::Image,
        MLN_STYLE_EXPRESSION_TYPE_PADDING => Type::Padding,
        MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION => Type::VariableAnchorOffsetCollection,
        _ => return None,
    })
}

/// C++ `to_native_type`.
fn type_of(expected: &mln_style_expression_type) -> Result<Type, Failure> {
    if expected.kind != MLN_STYLE_EXPRESSION_TYPE_ARRAY {
        return type_of_kind(expected.kind).ok_or_else(|| Failure::invalid_argument("expected_type.kind is unknown"));
    }
    if expected.item_kind == MLN_STYLE_EXPRESSION_TYPE_ARRAY {
        return Err(Failure::invalid_argument("expected_type.item_kind must not be an array"));
    }
    let item = type_of_kind(expected.item_kind)
        .ok_or_else(|| Failure::invalid_argument("expected_type.item_kind is unknown"))?;
    let count = (expected.item_count != 0).then_some(expected.item_count as usize);
    Ok(Array::with_n(item, count).into())
}

/// C++ `to_native_expected_type`.
///
/// # Safety
/// `options` is null or points at a readable `mln_style_expression_options`.
unsafe fn expected_type_of(options: *const mln_style_expression_options) -> Result<Option<Type>, Failure> {
    if options.is_null() {
        return Ok(None);
    }
    // SAFETY: the caller's contract.
    let Some(options) = (unsafe { read_sized(options) }) else {
        return Err(Failure::invalid_argument("mln_style_expression_options.size is too small"));
    };
    if options.fields & !MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE != 0 {
        return Err(Failure::invalid_argument("mln_style_expression_options.fields contains unknown bits"));
    }
    if options.fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE == 0 {
        return Ok(None);
    }
    type_of(&options.expected_type).map(Some)
}

/// # Safety
/// C API contract (style_expression.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_style_expression_create(
    expression: mln_buffer_view,
    options: *const mln_style_expression_options,
    out_expression: *mut mln_style_expression,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_expression.is_null() {
                return Err(Failure::invalid_argument("out_expression must not be null"));
            }
            if (*out_expression).0 != 0 {
                return Err(Failure::invalid_argument("*out_expression must be null"));
            }
            let expected = expected_type_of(options)?;
            let document = parse_json_view(expression, "expression")?;

            // GL JS createExpression() coerces a string result rather than
            // asserting it, as a string layer property does.
            let coerce = expected == Some(Type::String);
            let mut context = match expected {
                Some(expected) => ParsingContext::with_expected(expected),
                None => ParsingContext::new(),
            };
            let parsed =
                context.parse_expression(&Convertible::from(&document), coerce.then_some(TypeAnnotationOption::Coerce));
            let Some(parsed) = parsed else {
                return Err(Failure::invalid_argument(format!("expression: {}", context.get_combined_errors())));
            };
            // No value of another type holds a collator, so checking the
            // result type keeps every evaluation's result serializable.
            if *parsed.get_type() == Type::Collator {
                return Err(Failure::invalid_argument("expression: a collator result has no JSON form"));
            }
            let object = Arc::new(StyleExpression { expression: parsed });
            *out_expression = handles::insert(Object::StyleExpression(object)).into();
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (style_expression.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_style_expression_release(expression: mln_style_expression) {
    if expression.0 == 0 {
        return;
    }
    if let Some(Object::StyleExpression(_)) = handles::get(expression) {
        handles::remove(expression);
    }
}

/// # Safety
/// C API contract (style_expression.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_style_expression_get_info(
    expression: mln_style_expression,
    out_info: *mut mln_style_expression_info,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_info.is_null() {
                return Err(Failure::invalid_argument("out_info must not be null"));
            }
            if ((*out_info).size as usize) < size_of::<mln_style_expression_info>() {
                return Err(Failure::invalid_argument("mln_style_expression_info.size is too small"));
            }
            let object = lease(expression)?;
            (*out_info).is_feature_constant = is_feature_constant(&*object.expression);
            (*out_info).is_zoom_constant = is_zoom_constant(&*object.expression);
            Ok(())
        })
    }
}

/// The C++ core's `GeoJSONFeature`: a `GeometryTileFeature` view of a
/// GeoJSON feature, whose geometry is in the coordinates of the tile that the
/// inputs name, if they name one.
struct GeoJsonFeature {
    feature: Feature<f64>,
    geometry: GeometryCollection,
}

impl GeoJsonFeature {
    fn new(feature: Feature<f64>, canonical: Option<&CanonicalTileID>) -> GeoJsonFeature {
        let geometry = match canonical {
            Some(canonical) => {
                let geometry = convert_geometry_to_tile(&feature.geometry, canonical);
                // https://github.com/mapbox/geojson-vt-cpp/issues/44
                if to_feature_type(&feature.geometry) == FeatureType::Polygon {
                    fixup_polygons(&geometry).unwrap_or(geometry)
                } else {
                    geometry
                }
            }
            None => GeometryCollection::default(),
        };
        GeoJsonFeature { feature, geometry }
    }
}

impl GeometryTileFeature for GeoJsonFeature {
    fn get_type(&self) -> FeatureType {
        to_feature_type(&self.feature.geometry)
    }

    fn get_value(&self, key: &str) -> Option<CoreValue> {
        self.feature.properties.get(key).cloned()
    }

    fn get_properties(&self) -> &PropertyMap {
        &self.feature.properties
    }

    fn get_id(&self) -> Identifier {
        self.feature.id.clone()
    }

    fn get_geometries(&self) -> &GeometryCollection {
        &self.geometry
    }
}

/// The inputs that one evaluation reads, converted from their C form (C++
/// `EvaluationInputs`). The evaluation context borrows from it.
#[derive(Default)]
struct Inputs {
    zoom: Option<f32>,
    color_ramp_parameter: Option<f64>,
    elevation: Option<f32>,
    feature: Option<Feature<f64>>,
    feature_state: Option<PropertyMap>,
    global_state: Option<PropertyMap>,
    canonical: Option<CanonicalTileID>,
    available_images: BTreeSet<String>,
}

/// The value of a number input, which must be finite when `fields` sets it.
fn finite_input(fields: u32, field: u32, value: f64, name: &str) -> Result<Option<f64>, Failure> {
    if fields & field == 0 {
        return Ok(None);
    }
    if !value.is_finite() {
        return Err(Failure::invalid_argument(format!("{name} must be finite")));
    }
    Ok(Some(value))
}

/// C++ `json_object_input`.
fn json_object_input(view: mln_buffer_view, name: &str) -> Result<PropertyMap, Failure> {
    let document = parse_json_view(view, name)?;
    if !document.is_object() {
        return Err(Failure::invalid_argument(format!("{name} must contain a JSON object")));
    }
    match geojson::convert_value(&document) {
        CoreValue::Object(object) => Ok(object),
        _ => unreachable!(),
    }
}

/// C++ `to_native_feature`.
fn feature_input(view: mln_buffer_view) -> Result<Feature<f64>, Failure> {
    let document = parse_json_view(view, "feature")?;
    match geojson::convert(&document) {
        Ok(GeoJSON::Feature(feature)) => Ok(feature),
        Ok(_) => Err(Failure::invalid_argument("feature must contain one GeoJSON Feature")),
        Err(e) => Err(Failure::invalid_argument(format!("feature is invalid: {}", e.message()))),
    }
}

/// C++ `validate_canonical_tile_id` + `to_native_canonical_tile_id`.
fn canonical_input(tile_id: mln_canonical_tile_id) -> Result<CanonicalTileID, Failure> {
    if tile_id.z > 32 {
        return Err(Failure::invalid_argument("tile_id.z must be within [0, 32]"));
    }
    let limit = 1u64 << tile_id.z;
    if u64::from(tile_id.x) >= limit || u64::from(tile_id.y) >= limit {
        return Err(Failure::invalid_argument("tile_id x and y must be within zoom bounds"));
    }
    Ok(CanonicalTileID { z: tile_id.z as u8, x: tile_id.x, y: tile_id.y })
}

/// C++ `to_native_inputs`.
///
/// # Safety
/// `inputs` points at a readable `mln_style_expression_inputs` whose views
/// and `available_images` follow the C API contract.
unsafe fn inputs_of(inputs: *const mln_style_expression_inputs) -> Result<Inputs, Failure> {
    // SAFETY: the caller's contract.
    let Some(c) = (unsafe { read_sized(inputs) }) else {
        return Err(Failure::invalid_argument("mln_style_expression_inputs.size is too small"));
    };
    const KNOWN_FIELDS: u32 = MLN_STYLE_EXPRESSION_INPUT_ZOOM
        | MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER
        | MLN_STYLE_EXPRESSION_INPUT_ELEVATION
        | MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
    if c.fields & !KNOWN_FIELDS != 0 {
        return Err(Failure::invalid_argument("mln_style_expression_inputs.fields contains unknown bits"));
    }
    let mut out = Inputs {
        zoom: finite_input(c.fields, MLN_STYLE_EXPRESSION_INPUT_ZOOM, c.zoom, "zoom")?.map(|zoom| zoom as f32),
        color_ramp_parameter: finite_input(
            c.fields,
            MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER,
            c.color_ramp_parameter,
            "color_ramp_parameter",
        )?,
        elevation: finite_input(c.fields, MLN_STYLE_EXPRESSION_INPUT_ELEVATION, c.elevation, "elevation")?
            .map(|elevation| elevation as f32),
        ..Inputs::default()
    };
    if c.feature.size != 0 {
        out.feature = Some(feature_input(c.feature)?);
    }
    if c.feature_state.size != 0 {
        out.feature_state = Some(json_object_input(c.feature_state, "feature_state")?);
    }
    if c.global_state.size != 0 {
        out.global_state = Some(json_object_input(c.global_state, "global_state")?);
    }
    if c.fields & MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID != 0 {
        out.canonical = Some(canonical_input(c.canonical_tile_id)?);
    }
    if c.available_image_count != 0 && c.available_images.is_null() {
        return Err(Failure::invalid_argument(
            "available_images must not be null when available_image_count is nonzero",
        ));
    }
    for index in 0..c.available_image_count {
        // SAFETY: `available_images` holds `available_image_count` views.
        let image = unsafe { *c.available_images.add(index) };
        // SAFETY: C API contract for buffer views.
        let bytes = unsafe { image.bytes() };
        validate_bytes(bytes, image.size, "available image ID")?;
        out.available_images.insert(String::from_utf8_lossy(bytes.unwrap_or_default()).into_owned());
    }
    Ok(out)
}

/// JSON has no number that is not finite, so such a number becomes null, as
/// JSON.stringify() writes it (C++ `replace_non_finite_numbers`).
fn replace_non_finite_numbers(value: &mut CoreValue) {
    match value {
        CoreValue::Double(number) if !number.is_finite() => *value = CoreValue::Null,
        CoreValue::Array(items) => items.iter_mut().for_each(replace_non_finite_numbers),
        CoreValue::Object(members) => members.values_mut().for_each(replace_non_finite_numbers),
        _ => {}
    }
}

/// # Safety
/// C API contract (style_expression.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_style_expression_evaluate(
    expression: mln_style_expression,
    inputs: *const mln_style_expression_inputs,
    out_json: *mut std::ffi::c_char,
    json_capacity: usize,
    out_json_length: *mut usize,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_json.is_null() && json_capacity != 0 {
                return Err(Failure::invalid_argument("out_json must not be null when json_capacity is nonzero"));
            }
            if out_json_length.is_null() {
                return Err(Failure::invalid_argument("out_json_length must not be null"));
            }
            let object = lease(expression)?;
            let inputs = if inputs.is_null() { Inputs::default() } else { inputs_of(inputs)? };

            let feature = inputs.feature.map(|feature| GeoJsonFeature::new(feature, inputs.canonical.as_ref()));
            let context = EvaluationContext {
                zoom: inputs.zoom,
                feature: feature.as_ref().map(|feature| feature as &dyn GeometryTileFeature),
                color_ramp_parameter: inputs.color_ramp_parameter,
                elevation: inputs.elevation,
                feature_state: inputs.feature_state.as_ref(),
                available_images: Some(&inputs.available_images),
                canonical: inputs.canonical.as_ref(),
                global_state: inputs.global_state.as_ref(),
                ..EvaluationContext::default()
            };
            let result = object
                .expression
                .evaluate(&context)
                .map_err(|e| Failure::invalid_argument(format!("expression: {}", e.message)))?;
            let mut value = to_mbgl_value(&result);
            replace_non_finite_numbers(&mut value);
            let json = serialize_json(&value);

            *out_json_length = json.len();
            if json_capacity > json.len() {
                std::ptr::copy_nonoverlapping(json.as_ptr(), out_json.cast::<u8>(), json.len());
                *out_json.add(json.len()) = 0;
            }
            Ok(())
        })
    }
}
