use std::mem;
use std::ptr;
use std::ptr::NonNull;

use maplibre_native_sys as sys;

use crate::geojson::{Feature, feature_from_native};
use crate::json::{JsonValue, NativeJsonValue, json_value_from_native, json_value_try_to_native};
use crate::string::{StringView, string_view};
use crate::values::{ScreenBox, ScreenPoint, screen_point_to_native};
use crate::{Error, Result};

/// Source, feature, and state-key selector for render-session feature state.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct FeatureStateSelector {
    source_id: String,
    source_layer_id: Option<String>,
    feature_id: Option<String>,
    state_key: Option<String>,
}

impl FeatureStateSelector {
    pub fn new(source_id: impl Into<String>) -> Self {
        Self {
            source_id: source_id.into(),
            source_layer_id: None,
            feature_id: None,
            state_key: None,
        }
    }

    pub fn source_id(&self) -> &str {
        &self.source_id
    }

    pub fn source_layer_id(&self) -> Option<&str> {
        self.source_layer_id.as_deref()
    }

    pub fn feature_id(&self) -> Option<&str> {
        self.feature_id.as_deref()
    }

    pub fn state_key(&self) -> Option<&str> {
        self.state_key.as_deref()
    }

    pub fn with_source_layer_id(mut self, source_layer_id: impl Into<String>) -> Self {
        self.source_layer_id = Some(source_layer_id.into());
        self
    }

    pub fn with_feature_id(mut self, feature_id: impl Into<String>) -> Self {
        self.feature_id = Some(feature_id.into());
        self
    }

    pub fn with_state_key(mut self, state_key: impl Into<String>) -> Result<Self> {
        if self.feature_id.is_none() {
            return Err(Error::invalid_argument(
                "feature state selector state_key requires feature_id",
            ));
        }
        self.state_key = Some(state_key.into());
        Ok(self)
    }
}

pub struct NativeFeatureStateSelector<'a> {
    raw: sys::mln_feature_state_selector,
    _source_id: StringView<'a>,
    _source_layer_id: Option<StringView<'a>>,
    _feature_id: Option<StringView<'a>>,
    _state_key: Option<StringView<'a>>,
}

impl<'a> NativeFeatureStateSelector<'a> {
    fn new(selector: &'a FeatureStateSelector) -> Self {
        let source_id = string_view(&selector.source_id);
        let source_layer_id = selector.source_layer_id.as_deref().map(string_view);
        let feature_id = selector.feature_id.as_deref().map(string_view);
        let state_key = selector.state_key.as_deref().map(string_view);
        let mut fields = 0;
        if source_layer_id.is_some() {
            fields |= sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
        }
        if feature_id.is_some() {
            fields |= sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
        }
        if state_key.is_some() {
            fields |= sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
        }
        let raw = sys::mln_feature_state_selector {
            size: std::mem::size_of::<sys::mln_feature_state_selector>() as u32,
            fields,
            source_id: source_id.raw(),
            source_layer_id: source_layer_id.map_or(empty_string_view(), |view| view.raw()),
            feature_id: feature_id.map_or(empty_string_view(), |view| view.raw()),
            state_key: state_key.map_or(empty_string_view(), |view| view.raw()),
        };
        Self {
            raw,
            _source_id: source_id,
            _source_layer_id: source_layer_id,
            _feature_id: feature_id,
            _state_key: state_key,
        }
    }

    pub fn as_ptr(&self) -> *const sys::mln_feature_state_selector {
        &self.raw
    }
}

impl AsRef<sys::mln_feature_state_selector> for NativeFeatureStateSelector<'_> {
    fn as_ref(&self) -> &sys::mln_feature_state_selector {
        &self.raw
    }
}

pub fn feature_state_selector_to_native(
    selector: &FeatureStateSelector,
) -> NativeFeatureStateSelector<'_> {
    NativeFeatureStateSelector::new(selector)
}

#[doc(hidden)]
pub trait FeatureStateSelectorNativeExt {
    fn to_native(&self) -> NativeFeatureStateSelector<'_>;
}

impl FeatureStateSelectorNativeExt for FeatureStateSelector {
    fn to_native(&self) -> NativeFeatureStateSelector<'_> {
        feature_state_selector_to_native(self)
    }
}

fn empty_string_view() -> sys::mln_string_view {
    sys::mln_string_view {
        data: ptr::null(),
        size: 0,
    }
}

/// Screen-space geometry used for rendered feature queries.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum RenderedQueryGeometry {
    Point(ScreenPoint),
    Box(ScreenBox),
    LineString(Vec<ScreenPoint>),
}

impl RenderedQueryGeometry {
    pub fn point(point: ScreenPoint) -> Self {
        Self::Point(point)
    }

    pub fn box_(box_: ScreenBox) -> Self {
        Self::Box(box_)
    }

    pub fn line_string(points: Vec<ScreenPoint>) -> Self {
        Self::LineString(points)
    }
}

pub struct NativeRenderedQueryGeometry {
    raw: sys::mln_rendered_query_geometry,
    _points: Vec<sys::mln_screen_point>,
}

impl NativeRenderedQueryGeometry {
    fn new(geometry: &RenderedQueryGeometry) -> Self {
        match geometry {
            RenderedQueryGeometry::Point(point) => {
                // SAFETY: C constructor takes the point by value.
                let raw = unsafe {
                    sys::mln_rendered_query_geometry_point(screen_point_to_native(*point))
                };
                Self {
                    raw,
                    _points: Vec::new(),
                }
            }
            RenderedQueryGeometry::Box(box_) => {
                let raw_box = sys::mln_screen_box {
                    min: screen_point_to_native(box_.min),
                    max: screen_point_to_native(box_.max),
                };
                // SAFETY: C constructor takes the box by value.
                let raw = unsafe { sys::mln_rendered_query_geometry_box(raw_box) };
                Self {
                    raw,
                    _points: Vec::new(),
                }
            }
            RenderedQueryGeometry::LineString(points) => {
                let native_points = points
                    .iter()
                    .copied()
                    .map(screen_point_to_native)
                    .collect::<Vec<_>>();
                let ptr = const_ptr_or_null(&native_points);
                // SAFETY: ptr either is null for empty points or points to
                // native_points storage retained by this materializer.
                let raw = unsafe {
                    sys::mln_rendered_query_geometry_line_string(ptr, native_points.len())
                };
                Self {
                    raw,
                    _points: native_points,
                }
            }
        }
    }

    pub fn as_ptr(&self) -> *const sys::mln_rendered_query_geometry {
        &self.raw
    }
}

impl AsRef<sys::mln_rendered_query_geometry> for NativeRenderedQueryGeometry {
    fn as_ref(&self) -> &sys::mln_rendered_query_geometry {
        &self.raw
    }
}

pub fn rendered_query_geometry_to_native(
    geometry: &RenderedQueryGeometry,
) -> NativeRenderedQueryGeometry {
    NativeRenderedQueryGeometry::new(geometry)
}

#[doc(hidden)]
pub trait RenderedQueryGeometryNativeExt {
    fn to_native(&self) -> NativeRenderedQueryGeometry;
}

impl RenderedQueryGeometryNativeExt for RenderedQueryGeometry {
    fn to_native(&self) -> NativeRenderedQueryGeometry {
        rendered_query_geometry_to_native(self)
    }
}

/// Options for rendered feature queries.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct RenderedFeatureQueryOptions {
    pub layer_ids: Option<Vec<String>>,
    pub filter: Option<JsonValue>,
}

pub struct NativeRenderedFeatureQueryOptions<'a> {
    raw: sys::mln_rendered_feature_query_options,
    _layer_id_views: Vec<StringView<'a>>,
    _raw_layer_ids: Vec<sys::mln_string_view>,
    _filter: Option<NativeJsonValue>,
    _filter_raw: Option<Box<sys::mln_json_value>>,
}

impl<'a> NativeRenderedFeatureQueryOptions<'a> {
    fn new(options: &'a RenderedFeatureQueryOptions) -> Result<Self> {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_rendered_feature_query_options_default() };
        let mut layer_id_views = Vec::new();
        let mut raw_layer_ids = Vec::new();
        if let Some(layer_ids) = &options.layer_ids {
            raw.fields |= sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
            layer_id_views = layer_ids.iter().map(|id| string_view(id)).collect();
            raw_layer_ids = layer_id_views.iter().map(|view| view.raw()).collect();
            raw.layer_ids = const_ptr_or_null(&raw_layer_ids);
            raw.layer_id_count = raw_layer_ids.len();
        }
        let filter = options
            .filter
            .as_ref()
            .map(json_value_try_to_native)
            .transpose()?;
        let filter_raw = filter.as_ref().map(|filter| Box::new(*filter.as_ref()));
        if let Some(filter_raw) = &filter_raw {
            raw.filter = filter_raw.as_ref();
        }
        Ok(Self {
            raw,
            _layer_id_views: layer_id_views,
            _raw_layer_ids: raw_layer_ids,
            _filter: filter,
            _filter_raw: filter_raw,
        })
    }

    pub fn as_ptr(&self) -> *const sys::mln_rendered_feature_query_options {
        &self.raw
    }
}

impl AsRef<sys::mln_rendered_feature_query_options> for NativeRenderedFeatureQueryOptions<'_> {
    fn as_ref(&self) -> &sys::mln_rendered_feature_query_options {
        &self.raw
    }
}

pub fn rendered_feature_query_options_to_native(
    options: &RenderedFeatureQueryOptions,
) -> Result<NativeRenderedFeatureQueryOptions<'_>> {
    NativeRenderedFeatureQueryOptions::new(options)
}

#[doc(hidden)]
pub trait RenderedFeatureQueryOptionsNativeExt {
    fn to_native(&self) -> Result<NativeRenderedFeatureQueryOptions<'_>>;
}

impl RenderedFeatureQueryOptionsNativeExt for RenderedFeatureQueryOptions {
    fn to_native(&self) -> Result<NativeRenderedFeatureQueryOptions<'_>> {
        rendered_feature_query_options_to_native(self)
    }
}

/// Options for source feature queries.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct SourceFeatureQueryOptions {
    pub source_layer_ids: Option<Vec<String>>,
    pub filter: Option<JsonValue>,
}

pub struct NativeSourceFeatureQueryOptions<'a> {
    raw: sys::mln_source_feature_query_options,
    _source_layer_id_views: Vec<StringView<'a>>,
    _raw_source_layer_ids: Vec<sys::mln_string_view>,
    _filter: Option<NativeJsonValue>,
    _filter_raw: Option<Box<sys::mln_json_value>>,
}

impl<'a> NativeSourceFeatureQueryOptions<'a> {
    fn new(options: &'a SourceFeatureQueryOptions) -> Result<Self> {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_source_feature_query_options_default() };
        let mut source_layer_id_views = Vec::new();
        let mut raw_source_layer_ids = Vec::new();
        if let Some(source_layer_ids) = &options.source_layer_ids {
            raw.fields |= sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
            source_layer_id_views = source_layer_ids.iter().map(|id| string_view(id)).collect();
            raw_source_layer_ids = source_layer_id_views
                .iter()
                .map(|view| view.raw())
                .collect();
            raw.source_layer_ids = const_ptr_or_null(&raw_source_layer_ids);
            raw.source_layer_id_count = raw_source_layer_ids.len();
        }
        let filter = options
            .filter
            .as_ref()
            .map(json_value_try_to_native)
            .transpose()?;
        let filter_raw = filter.as_ref().map(|filter| Box::new(*filter.as_ref()));
        if let Some(filter_raw) = &filter_raw {
            raw.filter = filter_raw.as_ref();
        }
        Ok(Self {
            raw,
            _source_layer_id_views: source_layer_id_views,
            _raw_source_layer_ids: raw_source_layer_ids,
            _filter: filter,
            _filter_raw: filter_raw,
        })
    }

    pub fn as_ptr(&self) -> *const sys::mln_source_feature_query_options {
        &self.raw
    }
}

impl AsRef<sys::mln_source_feature_query_options> for NativeSourceFeatureQueryOptions<'_> {
    fn as_ref(&self) -> &sys::mln_source_feature_query_options {
        &self.raw
    }
}

pub fn source_feature_query_options_to_native(
    options: &SourceFeatureQueryOptions,
) -> Result<NativeSourceFeatureQueryOptions<'_>> {
    NativeSourceFeatureQueryOptions::new(options)
}

#[doc(hidden)]
pub trait SourceFeatureQueryOptionsNativeExt {
    fn to_native(&self) -> Result<NativeSourceFeatureQueryOptions<'_>>;
}

impl SourceFeatureQueryOptionsNativeExt for SourceFeatureQueryOptions {
    fn to_native(&self) -> Result<NativeSourceFeatureQueryOptions<'_>> {
        source_feature_query_options_to_native(self)
    }
}

/// One copied query result feature.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct QueriedFeature {
    pub feature: Feature,
    pub source_id: Option<String>,
    pub source_layer_id: Option<String>,
    pub state: Option<JsonValue>,
}

/// Copied feature-extension query result.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum FeatureExtensionResult {
    Value(JsonValue),
    FeatureCollection(Vec<Feature>),
    Unknown(u32),
}

/// Copies an owned native feature-query result into owned Rust data.
///
/// # Safety
///
/// `ptr` must point to a live `mln_feature_query_result` handle owned by the
/// caller and returned by the matching C API. This function takes ownership of
/// that handle and releases it before returning, including on copy errors.
pub unsafe fn copy_feature_query_result(
    ptr: NonNull<sys::mln_feature_query_result>,
) -> Result<Vec<QueriedFeature>> {
    // SAFETY: ptr is an owned query result returned by the C API and released by the guard.
    let result = unsafe { crate::handle::feature_query_result(ptr.as_ptr()) }?;
    let mut count = 0;
    // SAFETY: result is live and count points to writable storage.
    crate::check(unsafe { sys::mln_feature_query_result_count(result.as_ptr(), &mut count) })?;
    let mut features = Vec::with_capacity(count);
    for index in 0..count {
        let mut raw = sys::mln_queried_feature {
            size: mem::size_of::<sys::mln_queried_feature>() as u32,
            fields: 0,
            feature: empty_feature(),
            source_id: empty_string_view(),
            source_layer_id: empty_string_view(),
            state: ptr::null(),
        };
        // SAFETY: result is live, index is within count reported by native,
        // and raw points to initialized writable storage with size set.
        crate::check(unsafe {
            sys::mln_feature_query_result_get(result.as_ptr(), index, &mut raw)
        })?;
        // SAFETY: raw now contains result-owned views valid until result drops;
        // this copies all nested data before the next iteration/drop.
        features.push(unsafe { copy_queried_feature(&raw) }?);
    }
    Ok(features)
}

/// Copies a borrowed native queried feature descriptor into owned Rust data.
///
/// # Safety
///
/// `raw` and all nested pointers indicated by `raw.fields` must be valid for
/// the duration of this call. The returned value owns all copied data.
unsafe fn copy_queried_feature(raw: &sys::mln_queried_feature) -> Result<QueriedFeature> {
    // SAFETY: Caller promises raw.feature nested storage is valid for this call.
    let feature = unsafe { feature_from_native(&raw.feature, 0) }?;
    let source_id = if raw.fields & sys::MLN_QUERIED_FEATURE_SOURCE_ID != 0 {
        // SAFETY: Caller promises native string storage is valid.
        Some(unsafe { crate::string::copy_string_view(raw.source_id) }?)
    } else {
        None
    };
    let source_layer_id = if raw.fields & sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID != 0 {
        // SAFETY: Caller promises native string storage is valid.
        Some(unsafe { crate::string::copy_string_view(raw.source_layer_id) }?)
    } else {
        None
    };
    let state = if raw.fields & sys::MLN_QUERIED_FEATURE_STATE != 0 && !raw.state.is_null() {
        // SAFETY: Caller promises state storage is valid.
        Some(unsafe { json_value_from_native(&*raw.state) }?)
    } else {
        None
    };
    Ok(QueriedFeature {
        feature,
        source_id,
        source_layer_id,
        state,
    })
}

/// Copies an owned native feature-extension result into owned Rust data.
///
/// # Safety
///
/// `ptr` must point to a live `mln_feature_extension_result` handle owned by
/// the caller and returned by the matching C API. This function takes ownership
/// of that handle and releases it before returning, including on copy errors.
pub unsafe fn copy_feature_extension_result(
    ptr: NonNull<sys::mln_feature_extension_result>,
) -> Result<FeatureExtensionResult> {
    // SAFETY: ptr is an owned extension result returned by the C API and released by the guard.
    let result = unsafe { crate::handle::feature_extension_result(ptr.as_ptr()) }?;
    let mut info = sys::mln_feature_extension_result_info {
        size: mem::size_of::<sys::mln_feature_extension_result_info>() as u32,
        type_: 0,
        data: sys::mln_feature_extension_result_info__bindgen_ty_1 { value: ptr::null() },
    };
    // SAFETY: result is live and info points to initialized writable storage.
    crate::check(unsafe { sys::mln_feature_extension_result_get(result.as_ptr(), &mut info) })?;
    match info.type_ {
        sys::MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE => {
            // SAFETY: Active union member is selected by type_. Native returned
            // a value pointer valid until result drops; this copies it now.
            let value = unsafe { info.data.value };
            if value.is_null() {
                return Err(Error::invalid_argument(
                    "feature extension value result must not be null",
                ));
            }
            // SAFETY: value was checked non-null and is result-owned.
            Ok(FeatureExtensionResult::Value(unsafe {
                json_value_from_native(&*value)
            }?))
        }
        sys::MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION => {
            // SAFETY: Active union member is selected by type_.
            let collection = unsafe { info.data.feature_collection };
            let features = feature_collection_slice(
                collection.features,
                collection.feature_count,
                "feature extension feature collection",
            )?;
            let mut copied = Vec::with_capacity(features.len());
            for feature in features {
                // SAFETY: features came from validated collection storage.
                copied.push(unsafe { feature_from_native(feature, 1) }?);
            }
            Ok(FeatureExtensionResult::FeatureCollection(copied))
        }
        type_ => Ok(FeatureExtensionResult::Unknown(type_)),
    }
}

fn feature_collection_slice<'a>(
    ptr: *const sys::mln_feature,
    len: usize,
    context: &'static str,
) -> Result<&'a [sys::mln_feature]> {
    if len == 0 {
        return Ok(&[]);
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(format!(
            "{context} pointer must not be null when length is nonzero"
        )));
    }
    // SAFETY: ptr is non-null and caller/native reports len initialized entries.
    Ok(unsafe { std::slice::from_raw_parts(ptr, len) })
}

fn empty_feature() -> sys::mln_feature {
    sys::mln_feature {
        size: mem::size_of::<sys::mln_feature>() as u32,
        geometry: ptr::null(),
        properties: ptr::null(),
        property_count: 0,
        identifier_type: sys::MLN_FEATURE_IDENTIFIER_TYPE_NULL,
        identifier: sys::mln_feature__bindgen_ty_1 { uint_value: 0 },
    }
}

fn const_ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{JsonMember, JsonValue};

    #[test]
    fn feature_state_selector_materializes_fields_and_empty_views() {
        let selector = FeatureStateSelector::new("point")
            .with_source_layer_id("layer")
            .with_feature_id("feature-1")
            .with_state_key("hover")
            .unwrap();
        let native = feature_state_selector_to_native(&selector);
        let raw = native.as_ref();

        assert_eq!(raw.size as usize, std::mem::size_of_val(raw));
        assert_eq!(
            raw.fields,
            sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
                | sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
                | sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY
        );
        // SAFETY: The materializer keeps these views valid for this scope.
        assert_eq!(
            unsafe { crate::string::copy_string_view(raw.source_id) }.unwrap(),
            "point"
        );
        // SAFETY: The materializer keeps these views valid for this scope.
        assert_eq!(
            unsafe { crate::string::copy_string_view(raw.source_layer_id) }.unwrap(),
            "layer"
        );

        let source_only_selector = FeatureStateSelector::new("point");
        let source_only = feature_state_selector_to_native(&source_only_selector);
        let raw = source_only.as_ref();
        assert_eq!(raw.fields, 0);
        assert_eq!(raw.source_layer_id.size, 0);
        assert!(raw.source_layer_id.data.is_null());
    }

    #[test]
    fn rendered_query_geometry_materializes_variants_and_retains_line_storage() {
        let point = rendered_query_geometry_to_native(&RenderedQueryGeometry::point(
            ScreenPoint::new(1.0, 2.0),
        ));
        let raw = point.as_ref();
        assert_eq!(raw.type_, sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT);
        // SAFETY: Active union member is selected by type_.
        let raw_point = unsafe { raw.data.point };
        assert_eq!((raw_point.x, raw_point.y), (1.0, 2.0));

        let line = rendered_query_geometry_to_native(&RenderedQueryGeometry::line_string(vec![
            ScreenPoint::new(1.0, 2.0),
            ScreenPoint::new(3.0, 4.0),
        ]));
        let raw = line.as_ref();
        assert_eq!(raw.type_, sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING);
        // SAFETY: Active union member is selected by type_.
        let line_string = unsafe { raw.data.line_string };
        assert_eq!(line_string.point_count, 2);
        assert!(!line_string.points.is_null());
    }

    #[test]
    fn feature_extension_collection_slice_rejects_missing_nonempty_pointer() {
        let empty = feature_collection_slice(ptr::null(), 0, "test collection").unwrap();
        assert!(empty.is_empty());

        let Err(err) = feature_collection_slice(ptr::null(), 1, "test collection") else {
            panic!("missing nonempty feature collection pointer should fail");
        };
        assert!(
            err.to_string()
                .contains("test collection pointer must not be null")
        );
    }

    #[test]
    fn feature_query_options_materialize_arrays_filters_and_backing_storage() {
        let filter = JsonValue::object(vec![JsonMember::new(
            "kind",
            JsonValue::String("park".into()),
        )]);
        let rendered_options = RenderedFeatureQueryOptions {
            layer_ids: Some(vec!["point-circle".into()]),
            filter: Some(filter.clone()),
        };
        let rendered = rendered_feature_query_options_to_native(&rendered_options).unwrap();
        let raw = rendered.as_ref();
        assert_eq!(raw.size as usize, std::mem::size_of_val(raw));
        assert_eq!(raw.fields, sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS);
        assert_eq!(raw.layer_id_count, 1);
        assert!(!raw.layer_ids.is_null());
        assert!(!raw.filter.is_null());

        let source_options = SourceFeatureQueryOptions {
            source_layer_ids: Some(vec!["landuse".into()]),
            filter: Some(filter),
        };
        let source = source_feature_query_options_to_native(&source_options).unwrap();
        let raw = source.as_ref();
        assert_eq!(raw.size as usize, std::mem::size_of_val(raw));
        assert_eq!(
            raw.fields,
            sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
        );
        assert_eq!(raw.source_layer_id_count, 1);
        assert!(!raw.source_layer_ids.is_null());
        assert!(!raw.filter.is_null());
    }
}
