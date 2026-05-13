use std::os::raw::c_char;
use std::ptr;

use maplibre_native_sys as sys;

use crate::geometry::MAX_GEOMETRY_COLLECTION_DEPTH;
use crate::json::{JsonMember, NativeJsonMembers, copy_json_members};
use crate::{Error, Geometry, Result};

/// GeoJSON feature identifier value.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
#[derive(Default)]
pub enum FeatureIdentifier {
    #[default]
    Null,
    UInt(u64),
    Int(i64),
    Double(f64),
    String(String),
}

/// Owned GeoJSON feature descriptor.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct Feature {
    pub geometry: Geometry,
    pub properties: Vec<JsonMember>,
    pub identifier: FeatureIdentifier,
}

impl Feature {
    pub fn new(geometry: Geometry, properties: Vec<JsonMember>) -> Self {
        Self {
            geometry,
            properties,
            identifier: FeatureIdentifier::Null,
        }
    }

    pub fn with_identifier(mut self, identifier: FeatureIdentifier) -> Self {
        self.identifier = identifier;
        self
    }

    pub(crate) fn try_to_native(&self, depth: usize) -> Result<NativeFeature> {
        NativeFeature::new(self, depth)
    }

    /// Copies a borrowed native feature descriptor into an owned Rust value.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this
    /// call. The returned value owns all copied data.
    pub(crate) unsafe fn from_native(raw: &sys::mln_feature, depth: usize) -> Result<Self> {
        check_geojson_depth(depth)?;
        if raw.geometry.is_null() {
            return Err(Error::invalid_argument("feature geometry must not be null"));
        }
        // SAFETY: raw.geometry was checked non-null and the caller promises
        // nested feature storage is valid for this call.
        let geometry = unsafe { Geometry::from_native_with_depth(&*raw.geometry, depth + 1) }?;
        // SAFETY: The caller promises property storage is valid for this call.
        let properties =
            unsafe { copy_json_members(raw.properties, raw.property_count, depth + 1) }?;
        // SAFETY: Identifier union access is selected by identifier_type.
        let identifier = unsafe { copy_feature_identifier(raw) }?;
        Ok(Self {
            geometry,
            properties,
            identifier,
        })
    }
}

/// Owned GeoJSON descriptor tree.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum GeoJson {
    Geometry(Geometry),
    Feature(Feature),
    FeatureCollection(Vec<Feature>),
}

impl GeoJson {
    pub(crate) fn try_to_native(&self) -> Result<NativeGeoJson> {
        NativeGeoJson::new(self)
    }

    /// Copies a borrowed native GeoJSON descriptor into an owned Rust value.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this
    /// call. The returned value owns all copied data.
    #[cfg(test)]
    pub(crate) unsafe fn from_native(raw: &sys::mln_geojson) -> Result<Self> {
        match raw.type_ {
            sys::MLN_GEOJSON_TYPE_GEOMETRY => {
                // SAFETY: Active union member is selected by raw.type_.
                let geometry = unsafe { raw.data.geometry };
                if geometry.is_null() {
                    return Err(Error::invalid_argument("GeoJSON geometry must not be null"));
                }
                // SAFETY: geometry was checked non-null and caller promised nested storage.
                Ok(Self::Geometry(unsafe {
                    Geometry::from_native(&*geometry)
                }?))
            }
            sys::MLN_GEOJSON_TYPE_FEATURE => {
                // SAFETY: Active union member is selected by raw.type_.
                let feature = unsafe { raw.data.feature };
                if feature.is_null() {
                    return Err(Error::invalid_argument("GeoJSON feature must not be null"));
                }
                // SAFETY: feature was checked non-null and caller promised nested storage.
                Ok(Self::Feature(unsafe {
                    Feature::from_native(&*feature, 0)
                }?))
            }
            sys::MLN_GEOJSON_TYPE_FEATURE_COLLECTION => {
                // SAFETY: Active union member is selected by raw.type_.
                let collection = unsafe { raw.data.feature_collection };
                let features = feature_slice(
                    collection.features,
                    collection.feature_count,
                    "GeoJSON feature collection",
                )?;
                let mut copied = Vec::with_capacity(features.len());
                for feature in features {
                    // SAFETY: features came from validated collection storage.
                    copied.push(unsafe { Feature::from_native(feature, 1) }?);
                }
                Ok(Self::FeatureCollection(copied))
            }
            type_ => Err(Error::invalid_argument(format!(
                "unknown native GeoJSON type: {type_}"
            ))),
        }
    }
}

pub(crate) struct NativeFeature {
    raw: sys::mln_feature,
    _geometry: crate::geometry::NativeGeometry,
    _geometry_raw: Box<sys::mln_geometry>,
    _properties: NativeJsonMembers,
    identifier_string: Option<Box<[u8]>>,
}

impl NativeFeature {
    fn new(feature: &Feature, depth: usize) -> Result<Self> {
        check_geojson_depth(depth)?;
        let geometry = feature.geometry.try_to_native_with_depth(depth + 1)?;
        let geometry_raw = Box::new(*geometry.as_ref());
        let properties = NativeJsonMembers::new(&feature.properties, depth + 1)?;
        let mut native = Self {
            raw: sys::mln_feature {
                size: std::mem::size_of::<sys::mln_feature>() as u32,
                geometry: geometry_raw.as_ref(),
                properties: ptr::null(),
                property_count: 0,
                identifier_type: sys::MLN_FEATURE_IDENTIFIER_TYPE_NULL,
                identifier: sys::mln_feature__bindgen_ty_1 { uint_value: 0 },
            },
            _geometry: geometry,
            _geometry_raw: geometry_raw,
            _properties: properties,
            identifier_string: None,
        };
        native.raw.properties = native._properties.as_ptr();
        native.raw.property_count = native._properties.len();
        native.write_identifier(&feature.identifier)?;
        Ok(native)
    }

    pub(crate) fn as_ref(&self) -> &sys::mln_feature {
        &self.raw
    }

    pub(crate) fn as_ptr(&self) -> *const sys::mln_feature {
        &self.raw
    }

    fn write_identifier(&mut self, identifier: &FeatureIdentifier) -> Result<()> {
        match identifier {
            FeatureIdentifier::Null => {
                self.raw.identifier_type = sys::MLN_FEATURE_IDENTIFIER_TYPE_NULL;
            }
            FeatureIdentifier::UInt(value) => {
                self.raw.identifier_type = sys::MLN_FEATURE_IDENTIFIER_TYPE_UINT;
                self.raw.identifier = sys::mln_feature__bindgen_ty_1 { uint_value: *value };
            }
            FeatureIdentifier::Int(value) => {
                self.raw.identifier_type = sys::MLN_FEATURE_IDENTIFIER_TYPE_INT;
                self.raw.identifier = sys::mln_feature__bindgen_ty_1 { int_value: *value };
            }
            FeatureIdentifier::Double(value) => {
                if !value.is_finite() {
                    return Err(Error::invalid_argument(
                        "feature identifier double values must be finite",
                    ));
                }
                self.raw.identifier_type = sys::MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE;
                self.raw.identifier = sys::mln_feature__bindgen_ty_1 {
                    double_value: *value,
                };
            }
            FeatureIdentifier::String(value) => {
                let bytes = value.as_bytes().to_vec().into_boxed_slice();
                let view = sys::mln_string_view {
                    data: if bytes.is_empty() {
                        ptr::null()
                    } else {
                        bytes.as_ptr().cast::<c_char>()
                    },
                    size: bytes.len(),
                };
                self.identifier_string = Some(bytes);
                self.raw.identifier_type = sys::MLN_FEATURE_IDENTIFIER_TYPE_STRING;
                self.raw.identifier = sys::mln_feature__bindgen_ty_1 { string_value: view };
            }
        }
        Ok(())
    }
}

pub(crate) struct NativeGeoJson {
    raw: sys::mln_geojson,
    geometry: Option<crate::geometry::NativeGeometry>,
    geometry_raw: Option<Box<sys::mln_geometry>>,
    feature: Option<NativeFeature>,
    feature_raw: Option<Box<sys::mln_feature>>,
    features: Vec<NativeFeature>,
    raw_features: Option<Box<[sys::mln_feature]>>,
}

impl NativeGeoJson {
    fn new(geojson: &GeoJson) -> Result<Self> {
        let mut native = Self::empty(sys::MLN_GEOJSON_TYPE_GEOMETRY);
        match geojson {
            GeoJson::Geometry(geometry) => {
                let geometry = geometry.try_to_native()?;
                let geometry_raw = Box::new(*geometry.as_ref());
                native.raw.type_ = sys::MLN_GEOJSON_TYPE_GEOMETRY;
                native.raw.data = sys::mln_geojson__bindgen_ty_1 {
                    geometry: geometry_raw.as_ref(),
                };
                native.geometry = Some(geometry);
                native.geometry_raw = Some(geometry_raw);
            }
            GeoJson::Feature(feature) => {
                let feature = feature.try_to_native(0)?;
                let feature_raw = Box::new(*feature.as_ref());
                native.raw.type_ = sys::MLN_GEOJSON_TYPE_FEATURE;
                native.raw.data = sys::mln_geojson__bindgen_ty_1 {
                    feature: feature_raw.as_ref(),
                };
                native.feature = Some(feature);
                native.feature_raw = Some(feature_raw);
            }
            GeoJson::FeatureCollection(features) => {
                native.raw.type_ = sys::MLN_GEOJSON_TYPE_FEATURE_COLLECTION;
                native.features = features
                    .iter()
                    .map(|feature| feature.try_to_native(1))
                    .collect::<Result<Vec<_>>>()?;
                let raw_features = native
                    .features
                    .iter()
                    .map(|feature| feature.raw)
                    .collect::<Vec<_>>()
                    .into_boxed_slice();
                native.raw.data = sys::mln_geojson__bindgen_ty_1 {
                    feature_collection: sys::mln_feature_collection {
                        features: ptr_or_null(raw_features.as_ref()),
                        feature_count: raw_features.len(),
                    },
                };
                native.raw_features = Some(raw_features);
            }
        }
        Ok(native)
    }

    fn empty(type_: u32) -> Self {
        Self {
            raw: sys::mln_geojson {
                size: std::mem::size_of::<sys::mln_geojson>() as u32,
                type_,
                data: sys::mln_geojson__bindgen_ty_1 {
                    geometry: ptr::null(),
                },
            },
            geometry: None,
            geometry_raw: None,
            feature: None,
            feature_raw: None,
            features: Vec::new(),
            raw_features: None,
        }
    }

    pub(crate) fn as_ptr(&self) -> *const sys::mln_geojson {
        &self.raw
    }

    #[cfg(test)]
    pub(crate) fn as_ref(&self) -> &sys::mln_geojson {
        &self.raw
    }
}

unsafe fn copy_feature_identifier(raw: &sys::mln_feature) -> Result<FeatureIdentifier> {
    match raw.identifier_type {
        sys::MLN_FEATURE_IDENTIFIER_TYPE_NULL => Ok(FeatureIdentifier::Null),
        // SAFETY: Active union member is selected by identifier_type.
        sys::MLN_FEATURE_IDENTIFIER_TYPE_UINT => Ok(FeatureIdentifier::UInt(unsafe {
            raw.identifier.uint_value
        })),
        // SAFETY: Active union member is selected by identifier_type.
        sys::MLN_FEATURE_IDENTIFIER_TYPE_INT => {
            Ok(FeatureIdentifier::Int(unsafe { raw.identifier.int_value }))
        }
        // SAFETY: Active union member is selected by identifier_type.
        sys::MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE => Ok(FeatureIdentifier::Double(unsafe {
            raw.identifier.double_value
        })),
        sys::MLN_FEATURE_IDENTIFIER_TYPE_STRING => {
            // SAFETY: Active union member is selected by identifier_type.
            let view = unsafe { raw.identifier.string_value };
            // SAFETY: Caller promises string storage is valid for this call.
            Ok(FeatureIdentifier::String(unsafe {
                maplibre_native_support::string::copy_string_view(view)
            }?))
        }
        type_ => Err(Error::invalid_argument(format!(
            "unknown native feature identifier type: {type_}"
        ))),
    }
}

#[cfg(test)]
fn feature_slice<'a>(
    ptr: *const sys::mln_feature,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_feature]> {
    if count == 0 {
        return Ok(&[]);
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(format!(
            "{name} pointer must not be null when count is nonzero"
        )));
    }
    // SAFETY: The caller promises ptr points to count valid features.
    Ok(unsafe { std::slice::from_raw_parts(ptr, count) })
}

fn check_geojson_depth(depth: usize) -> Result<()> {
    if depth > MAX_GEOMETRY_COLLECTION_DEPTH {
        Err(Error::invalid_argument(format!(
            "GeoJSON descriptor depth exceeds {MAX_GEOMETRY_COLLECTION_DEPTH}"
        )))
    } else {
        Ok(())
    }
}

fn ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{JsonMember, JsonValue, LatLng};

    #[test]
    fn feature_and_geojson_materialize_and_copy_back() {
        let feature = Feature::new(
            Geometry::Point(LatLng::new(1.0, 2.0)),
            vec![
                JsonMember::new("name", JsonValue::String("park".to_owned())),
                JsonMember::new("name", JsonValue::String("duplicate".to_owned())),
            ],
        )
        .with_identifier(FeatureIdentifier::UInt(u64::MAX));
        let geojson = GeoJson::FeatureCollection(vec![feature.clone()]);

        let native_feature = feature.try_to_native(0).unwrap();
        // SAFETY: native_feature owns a valid descriptor graph for this test.
        let copied_feature = unsafe { Feature::from_native(&native_feature.raw, 0) }.unwrap();
        let native_geojson = geojson.try_to_native().unwrap();
        // SAFETY: native_geojson owns a valid descriptor graph for this test.
        let copied_geojson = unsafe { GeoJson::from_native(native_geojson.as_ref()) }.unwrap();

        assert_eq!(copied_feature, feature);
        assert_eq!(copied_geojson, geojson);
    }

    #[test]
    fn geojson_depth_limit_counts_feature_geometry_boundary() {
        let mut geometry = Geometry::Empty;
        for _ in 0..(MAX_GEOMETRY_COLLECTION_DEPTH - 1) {
            geometry = Geometry::GeometryCollection(vec![geometry]);
        }
        geometry.try_to_native().unwrap();

        let feature = Feature::new(geometry.clone(), Vec::new());
        feature.try_to_native(0).unwrap();

        let geojson = GeoJson::FeatureCollection(vec![feature]);
        let error = geojson.try_to_native().err().unwrap();

        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert!(
            error
                .diagnostic()
                .contains("geometry collection depth exceeds")
        );
    }

    #[test]
    fn geojson_rejects_non_finite_feature_identifier_before_calling_c() {
        let feature = Feature::new(Geometry::Empty, Vec::new())
            .with_identifier(FeatureIdentifier::Double(f64::INFINITY));

        let error = feature.try_to_native(0).err().unwrap();

        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("finite"));
    }
}
