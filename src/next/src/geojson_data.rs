// Ports maplibre-native-ffi src/geojson/geojson_source_data.cpp and the
// GeoJSON parsing of src/geojson/geojson.cpp onto mln_style's GeoJSON
// source: the options of a GeoJSON source and the data prepared for it
// (`mln_geo_json_source_data`).
//
// Prepared data is immutable, so any thread may read or release it. The
// core's geojson-vt index already serializes its tile slicing with a mutex,
// so the data needs no wrapper (C++ `SerializedGeoJsonData`) to serve several
// sources.

use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::types::*;
use maplibre_native_next::geojson::GeoJSON;
use maplibre_native_next::maplibre_native_base::Geometry;
use maplibre_native_next::mln_style::conversion::{Converter, Convertible, Error};
use maplibre_native_next::mln_style::sources::geojson_source::{
    ClusterProperties, GeoJSONData, GeoJSONOptions, create_geojson_data, get_sequenced,
};
use maplibre_native_next::mln_util::immutable::Immutable;
use std::sync::Arc;

/// The data behind an `mln_geo_json_source_data` handle (C++
/// `GeoJsonSourceDataObject`).
pub struct PreparedGeoJson {
    pub data: Arc<dyn GeoJSONData>,
    pub options: Immutable<GeoJSONOptions>,
}

/// An `mln_geo_json_source_options` with every unset field at its default
/// (C++ `effective_geojson_source_options`), owning its cluster properties.
#[derive(Clone)]
pub struct GeoJsonOptions {
    min_zoom: f64,
    max_zoom: f64,
    tolerance: f64,
    cluster_max_zoom: f64,
    /// The `clusterProperties` JSON, or `None` for a view with null data and
    /// a nonzero size.
    cluster_properties: Option<Vec<u8>>,
    buffer: u32,
    cluster_radius: u32,
    cluster_min_points: u32,
    line_metrics: bool,
    cluster: bool,
    synchronous_tiling: bool,
}

impl Default for GeoJsonOptions {
    /// C++ `geojson_source_options_default`: `style::GeoJSONOptions{}`.
    fn default() -> Self {
        let defaults = GeoJSONOptions::default();
        GeoJsonOptions {
            min_zoom: f64::from(defaults.minzoom),
            max_zoom: f64::from(defaults.maxzoom),
            tolerance: defaults.tolerance,
            cluster_max_zoom: f64::from(defaults.cluster_max_zoom),
            cluster_properties: Some(Vec::new()),
            buffer: u32::from(defaults.buffer),
            cluster_radius: u32::from(defaults.cluster_radius),
            cluster_min_points: defaults.cluster_min_points as u32,
            line_metrics: defaults.line_metrics,
            cluster: defaults.cluster,
            synchronous_tiling: defaults.synchronous_update,
        }
    }
}

const KNOWN_OPTION_FIELDS: u32 = MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM
    | MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM
    | MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE
    | MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
    | MLN_GEO_JSON_SOURCE_OPTION_BUFFER
    | MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS
    | MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
    | MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS
    | MLN_GEO_JSON_SOURCE_OPTION_CLUSTER
    | MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING;

fn whole_zoom(zoom: f64, maximum: f64) -> bool {
    zoom.is_finite() && (0.0..=maximum).contains(&zoom) && zoom.floor() == zoom
}

impl GeoJsonOptions {
    /// C++ `validate_and_resolve_geojson_source_options`: the effective
    /// options of a caller's options, which may be null.
    ///
    /// # Safety
    /// `options` is null or points at a readable `mln_geo_json_source_options`
    /// whose `cluster_properties` view is valid.
    pub unsafe fn resolve(options: *const mln_geo_json_source_options) -> Result<GeoJsonOptions, Failure> {
        let mut effective = GeoJsonOptions::default();
        if options.is_null() {
            return Ok(effective);
        }
        // SAFETY: the caller's contract.
        let Some(options) = (unsafe { read_sized(options) }) else {
            return Err(Failure::invalid_argument("mln_geo_json_source_options.size is too small"));
        };
        if options.fields & !KNOWN_OPTION_FIELDS != 0 {
            return Err(Failure::invalid_argument("mln_geo_json_source_options.fields contains unknown bits"));
        }
        let has = |field: u32| options.fields & field != 0;
        if has(MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM) {
            effective.min_zoom = options.min_zoom;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM) {
            effective.max_zoom = options.max_zoom;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE) {
            effective.tolerance = options.tolerance;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) {
            effective.cluster_max_zoom = options.cluster_max_zoom;
        }
        // SAFETY: the caller's contract.
        effective.cluster_properties = unsafe { options.cluster_properties.bytes() }.map(<[u8]>::to_vec);
        if has(MLN_GEO_JSON_SOURCE_OPTION_BUFFER) {
            effective.buffer = options.buffer;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS) {
            effective.cluster_radius = options.cluster_radius;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) {
            effective.cluster_min_points = options.cluster_min_points;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS) {
            effective.line_metrics = options.line_metrics;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_CLUSTER) {
            effective.cluster = options.cluster;
        }
        if has(MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING) {
            effective.synchronous_tiling = options.synchronous_tiling;
        }

        if !whole_zoom(effective.max_zoom, 24.0) {
            return Err(Failure::invalid_argument("max_zoom must be an integer within [0, 24]"));
        }
        for (zoom, maximum, name) in
            [(effective.min_zoom, 24.0, "min_zoom"), (effective.cluster_max_zoom, 255.0, "cluster_max_zoom")]
        {
            if !whole_zoom(zoom, maximum) {
                return Err(Failure::invalid_argument(format!("{name} must be an integer within [0, {maximum}]")));
            }
        }
        if effective.min_zoom > effective.max_zoom {
            return Err(Failure::invalid_argument("min_zoom must be less than or equal to max_zoom"));
        }
        if !(effective.tolerance.is_finite() && effective.tolerance >= 0.0) {
            return Err(Failure::invalid_argument("tolerance must be finite and non-negative"));
        }
        if effective.buffer > 512 {
            return Err(Failure::invalid_argument("buffer must be at most 512"));
        }
        if effective.cluster_radius > 4095 {
            return Err(Failure::invalid_argument("cluster_radius must be at most 4095"));
        }
        Ok(effective)
    }

    /// C++ `to_native_geojson_source_options`. Cluster properties reach the
    /// core's `GeoJSONOptions` converter as a one-member object, which parses
    /// the aggregation expressions.
    pub fn to_native(&self) -> Result<Immutable<GeoJSONOptions>, Failure> {
        let mut native = GeoJSONOptions {
            minzoom: self.min_zoom as u8,
            maxzoom: self.max_zoom as u8,
            buffer: self.buffer as u16,
            tolerance: self.tolerance,
            line_metrics: self.line_metrics,
            cluster: self.cluster,
            cluster_radius: self.cluster_radius as u16,
            cluster_max_zoom: self.cluster_max_zoom as u8,
            cluster_min_points: self.cluster_min_points as usize,
            synchronous_update: self.synchronous_tiling,
            ..GeoJSONOptions::default()
        };
        let Some(cluster_properties) = &self.cluster_properties else {
            return Err(Failure::invalid_argument("cluster_properties data must not be null"));
        };
        if !cluster_properties.is_empty() {
            let document = parse_json_document(cluster_properties, "cluster_properties")?;
            if !document.is_object() {
                return Err(Failure::invalid_argument("cluster_properties must contain a JSON object"));
            }
            let wrapper = serde_json::json!({ "clusterProperties": document });
            let mut error = Error::default();
            let Some(converted) = GeoJSONOptions::convert(&Convertible::from(&wrapper), &mut error) else {
                return Err(Failure::invalid_argument(format!("GeoJSON source options: {}", error.message)));
            };
            native.cluster_properties = converted.cluster_properties;
        }
        Ok(Arc::new(native))
    }
}

/// C++ `cluster_properties_equal`: aggregations compare as parsed
/// expressions, so options prepared from equivalent JSON compare equal.
fn cluster_properties_equal(left: &ClusterProperties, right: &ClusterProperties) -> bool {
    left.len() == right.len()
        && left.iter().all(|(key, (map, reduce))| {
            right.get(key).is_some_and(|(other_map, other_reduce)| **map == **other_map && **reduce == **other_reduce)
        })
}

/// C++ `geojson_source_options_equal`.
pub fn options_equal(left: &GeoJSONOptions, right: &GeoJSONOptions) -> bool {
    left.minzoom == right.minzoom
        && left.maxzoom == right.maxzoom
        && left.buffer == right.buffer
        && left.tolerance == right.tolerance
        && left.line_metrics == right.line_metrics
        && left.cluster == right.cluster
        && left.cluster_radius == right.cluster_radius
        && left.cluster_max_zoom == right.cluster_max_zoom
        && left.cluster_min_points == right.cluster_min_points
        && left.synchronous_update == right.synchronous_update
        && cluster_properties_equal(&left.cluster_properties, &right.cluster_properties)
}

/// C++ `validate_bytes`.
fn validate_bytes(bytes: &[u8], name: &str) -> Result<(), Failure> {
    if bytes.is_empty() {
        return Err(Failure::invalid_argument(format!("{name} must not be empty")));
    }
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument(format!("{name} must not contain embedded NUL")));
    }
    Ok(())
}

/// C++ `parse_json_document`. A syntax error carries serde_json's message
/// where C++ has rapidjson's, as in the core.
pub fn parse_json_document(bytes: &[u8], name: &str) -> Result<serde_json::Value, Failure> {
    validate_bytes(bytes, name)?;
    serde_json::from_slice(bytes).map_err(|e| Failure::invalid_argument(format!("{name} is not valid JSON: {e}")))
}

/// C++ `to_native_geojson`: a GeoJSON feature collection, feature, or
/// geometry. `bytes` is `None` for a view with null data and a nonzero size.
pub fn parse_geojson(bytes: Option<&[u8]>) -> Result<GeoJSON, Failure> {
    let Some(bytes) = bytes else {
        return Err(Failure::invalid_argument("GeoJSON data must not be null"));
    };
    let document = parse_json_document(bytes, "GeoJSON")?;
    let mut error = Error::default();
    GeoJSON::convert(&Convertible::from(&document), &mut error)
        .ok_or_else(|| Failure::invalid_argument(format!("GeoJSON is invalid: {}", error.message)))
}

fn geometry_type_name(geometry: &Geometry<f64>) -> &'static str {
    match geometry {
        Geometry::Empty => "empty",
        Geometry::Point(_) => "point",
        Geometry::LineString(_) => "line string",
        Geometry::Polygon(_) => "polygon",
        Geometry::MultiPoint(_) => "multi-point",
        Geometry::MultiLineString(_) => "multi-line string",
        Geometry::MultiPolygon(_) => "multi-polygon",
        Geometry::GeometryCollection(_) => "geometry collection",
    }
}

/// C++ `validate_clustered_geojson`: clustering takes a feature collection
/// whose every feature is a point (an empty collection too).
fn validate_clustered(geojson: &GeoJSON) -> Result<(), Failure> {
    let features = match geojson {
        GeoJSON::FeatureCollection(features) => features,
        other => {
            let name = match other {
                GeoJSON::Geometry(_) => "a bare geometry",
                _ => "a single feature",
            };
            return Err(Failure::invalid_argument(format!(
                "clustered GeoJSON data requires a feature collection; the data is {name}"
            )));
        }
    };
    for (index, feature) in features.iter().enumerate() {
        if !matches!(feature.geometry, Geometry::Point(_)) {
            return Err(Failure::invalid_argument(format!(
                "clustered GeoJSON data requires point geometry on every feature; feature {index} has {} geometry",
                geometry_type_name(&feature.geometry)
            )));
        }
    }
    Ok(())
}

/// The prepared data that `handle` names, or the handle's fault.
pub fn lookup(handle: mln_geo_json_source_data) -> Result<Arc<PreparedGeoJson>, Failure> {
    match handles::resolve(handles::Kind::GeoJsonSourceData, handle)? {
        Object::GeoJsonSourceData(data) => Ok(data),
        _ => unreachable!(),
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_geo_json_source_data_create(
    data: mln_buffer_view,
    options: *const mln_geo_json_source_options,
    out_data: *mut mln_geo_json_source_data,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_data.is_null() {
                return Err(Failure::invalid_argument("out_data must not be null"));
            }
            if (*out_data).0 != 0 {
                return Err(Failure::invalid_argument("*out_data must be null"));
            }
            let effective = GeoJsonOptions::resolve(options)?;
            let geojson = parse_geojson(data.bytes())?;
            if effective.cluster {
                validate_clustered(&geojson)?;
            }
            let options = effective.to_native()?;
            let data = create_geojson_data(&geojson, get_sequenced(), &options).map_err(Failure::invalid_argument)?;
            *out_data = handles::insert(Object::GeoJsonSourceData(Arc::new(PreparedGeoJson { data, options }))).into();
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_geo_json_source_data_release(data: mln_geo_json_source_data) {
    if let Some(Object::GeoJsonSourceData(_)) = handles::get(data) {
        handles::remove(data);
    }
}
