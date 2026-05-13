use std::mem;
use std::ptr::{self, NonNull};

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::Result;
use crate::geojson::Feature;
use crate::json::JsonValue;
use crate::values::{ScreenBox, ScreenPoint};

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

    pub fn without_source_layer_id(mut self) -> Self {
        self.source_layer_id = None;
        self
    }

    pub fn with_feature_id(mut self, feature_id: impl Into<String>) -> Self {
        self.feature_id = Some(feature_id.into());
        self
    }

    pub fn without_feature_id(mut self) -> Self {
        self.feature_id = None;
        self.state_key = None;
        self
    }

    pub fn with_state_key(mut self, state_key: impl Into<String>) -> Result<Self> {
        if self.feature_id.is_none() {
            return Err(crate::Error::invalid_argument(
                "feature state selector state_key requires feature_id",
            ));
        }
        self.state_key = Some(state_key.into());
        Ok(self)
    }

    pub fn without_state_key(mut self) -> Self {
        self.state_key = None;
        self
    }

    pub(crate) fn to_native(&self) -> NativeFeatureStateSelector<'_> {
        NativeFeatureStateSelector::new(self)
    }
}

pub(crate) struct NativeFeatureStateSelector<'a> {
    raw: sys::mln_feature_state_selector,
    _source_id: support::string::StringView<'a>,
    _source_layer_id: Option<support::string::StringView<'a>>,
    _feature_id: Option<support::string::StringView<'a>>,
    _state_key: Option<support::string::StringView<'a>>,
}

impl<'a> NativeFeatureStateSelector<'a> {
    fn new(selector: &'a FeatureStateSelector) -> Self {
        let source_id = support::string::string_view(&selector.source_id);
        let source_layer_id = selector
            .source_layer_id
            .as_deref()
            .map(support::string::string_view);
        let feature_id = selector
            .feature_id
            .as_deref()
            .map(support::string::string_view);
        let state_key = selector
            .state_key
            .as_deref()
            .map(support::string::string_view);
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
            size: mem::size_of::<sys::mln_feature_state_selector>() as u32,
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

    pub(crate) fn as_ptr(&self) -> *const sys::mln_feature_state_selector {
        &self.raw
    }

    #[cfg(test)]
    pub(crate) fn as_ref(&self) -> &sys::mln_feature_state_selector {
        &self.raw
    }
}

fn empty_string_view() -> sys::mln_string_view {
    sys::mln_string_view {
        data: std::ptr::null(),
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

    pub(crate) fn to_native(&self) -> NativeRenderedQueryGeometry {
        NativeRenderedQueryGeometry::new(self)
    }
}

pub(crate) struct NativeRenderedQueryGeometry {
    raw: sys::mln_rendered_query_geometry,
    _points: Vec<sys::mln_screen_point>,
}

impl NativeRenderedQueryGeometry {
    fn new(geometry: &RenderedQueryGeometry) -> Self {
        match geometry {
            RenderedQueryGeometry::Point(point) => {
                // SAFETY: C constructor takes the point by value.
                let raw = unsafe { sys::mln_rendered_query_geometry_point(point.to_native()) };
                Self {
                    raw,
                    _points: Vec::new(),
                }
            }
            RenderedQueryGeometry::Box(box_) => {
                let raw_box = sys::mln_screen_box {
                    min: box_.min.to_native(),
                    max: box_.max.to_native(),
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
                    .map(ScreenPoint::to_native)
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

    pub(crate) fn as_ptr(&self) -> *const sys::mln_rendered_query_geometry {
        &self.raw
    }

    #[cfg(test)]
    pub(crate) fn as_ref(&self) -> &sys::mln_rendered_query_geometry {
        &self.raw
    }
}

/// Options for rendered feature queries.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct RenderedFeatureQueryOptions {
    pub layer_ids: Option<Vec<String>>,
    pub filter: Option<JsonValue>,
}

impl RenderedFeatureQueryOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_layer_ids(mut self, layer_ids: Vec<String>) -> Self {
        self.layer_ids = Some(layer_ids);
        self
    }

    pub fn without_layer_ids(mut self) -> Self {
        self.layer_ids = None;
        self
    }

    pub fn with_filter(mut self, filter: JsonValue) -> Self {
        self.filter = Some(filter);
        self
    }

    pub fn without_filter(mut self) -> Self {
        self.filter = None;
        self
    }

    pub(crate) fn to_native(&self) -> Result<NativeRenderedFeatureQueryOptions<'_>> {
        NativeRenderedFeatureQueryOptions::new(self)
    }
}

pub(crate) struct NativeRenderedFeatureQueryOptions<'a> {
    raw: sys::mln_rendered_feature_query_options,
    _layer_id_views: Vec<support::string::StringView<'a>>,
    _raw_layer_ids: Vec<sys::mln_string_view>,
    _filter: Option<crate::json::NativeJsonValue>,
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
            layer_id_views = layer_ids
                .iter()
                .map(|id| support::string::string_view(id))
                .collect();
            raw_layer_ids = layer_id_views.iter().map(|view| view.raw()).collect();
            raw.layer_ids = const_ptr_or_null(&raw_layer_ids);
            raw.layer_id_count = raw_layer_ids.len();
        }
        let filter = options
            .filter
            .as_ref()
            .map(JsonValue::try_to_native)
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

    pub(crate) fn as_ptr(&self) -> *const sys::mln_rendered_feature_query_options {
        &self.raw
    }

    #[cfg(test)]
    pub(crate) fn as_ref(&self) -> &sys::mln_rendered_feature_query_options {
        &self.raw
    }
}

/// Options for source feature queries.
#[derive(Debug, Clone, PartialEq, Default)]
#[non_exhaustive]
pub struct SourceFeatureQueryOptions {
    pub source_layer_ids: Option<Vec<String>>,
    pub filter: Option<JsonValue>,
}

impl SourceFeatureQueryOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_source_layer_ids(mut self, source_layer_ids: Vec<String>) -> Self {
        self.source_layer_ids = Some(source_layer_ids);
        self
    }

    pub fn without_source_layer_ids(mut self) -> Self {
        self.source_layer_ids = None;
        self
    }

    pub fn with_filter(mut self, filter: JsonValue) -> Self {
        self.filter = Some(filter);
        self
    }

    pub fn without_filter(mut self) -> Self {
        self.filter = None;
        self
    }

    pub(crate) fn to_native(&self) -> Result<NativeSourceFeatureQueryOptions<'_>> {
        NativeSourceFeatureQueryOptions::new(self)
    }
}

pub(crate) struct NativeSourceFeatureQueryOptions<'a> {
    raw: sys::mln_source_feature_query_options,
    _source_layer_id_views: Vec<support::string::StringView<'a>>,
    _raw_source_layer_ids: Vec<sys::mln_string_view>,
    _filter: Option<crate::json::NativeJsonValue>,
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
            source_layer_id_views = source_layer_ids
                .iter()
                .map(|id| support::string::string_view(id))
                .collect();
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
            .map(JsonValue::try_to_native)
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

    pub(crate) fn as_ptr(&self) -> *const sys::mln_source_feature_query_options {
        &self.raw
    }

    #[cfg(test)]
    pub(crate) fn as_ref(&self) -> &sys::mln_source_feature_query_options {
        &self.raw
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

pub(crate) struct FeatureQueryResultHandle(NonNull<sys::mln_feature_query_result>);

impl FeatureQueryResultHandle {
    pub(crate) fn new(ptr: NonNull<sys::mln_feature_query_result>) -> Self {
        Self(ptr)
    }

    fn as_ptr(&self) -> *const sys::mln_feature_query_result {
        self.0.as_ptr()
    }
}

impl Drop for FeatureQueryResultHandle {
    fn drop(&mut self) {
        // SAFETY: self.0 is an owned feature-query result handle.
        unsafe { sys::mln_feature_query_result_destroy(self.0.as_ptr()) };
    }
}

pub(crate) struct FeatureExtensionResultHandle(NonNull<sys::mln_feature_extension_result>);

impl FeatureExtensionResultHandle {
    pub(crate) fn new(ptr: NonNull<sys::mln_feature_extension_result>) -> Self {
        Self(ptr)
    }

    fn as_ptr(&self) -> *const sys::mln_feature_extension_result {
        self.0.as_ptr()
    }
}

impl Drop for FeatureExtensionResultHandle {
    fn drop(&mut self) {
        // SAFETY: self.0 is an owned feature-extension result handle.
        unsafe { sys::mln_feature_extension_result_destroy(self.0.as_ptr()) };
    }
}

fn const_ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

pub(crate) fn copy_feature_query_result(
    result: FeatureQueryResultHandle,
) -> Result<Vec<QueriedFeature>> {
    let mut count = 0;
    // SAFETY: result is live and count points to writable storage.
    support::check(unsafe { sys::mln_feature_query_result_count(result.as_ptr(), &mut count) })?;
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
        support::check(unsafe {
            sys::mln_feature_query_result_get(result.as_ptr(), index, &mut raw)
        })?;
        // SAFETY: raw now contains result-owned views valid until result drops;
        // this copies all nested data before the next iteration/drop.
        features.push(unsafe { copy_queried_feature(&raw) }?);
    }
    Ok(features)
}

unsafe fn copy_queried_feature(raw: &sys::mln_queried_feature) -> Result<QueriedFeature> {
    // SAFETY: Caller promises raw.feature nested storage is valid for this call.
    let feature = unsafe { Feature::from_native(&raw.feature, 0) }?;
    let source_id = if raw.fields & sys::MLN_QUERIED_FEATURE_SOURCE_ID != 0 {
        // SAFETY: Caller promises native string storage is valid.
        Some(unsafe { support::string::copy_string_view(raw.source_id) }?)
    } else {
        None
    };
    let source_layer_id = if raw.fields & sys::MLN_QUERIED_FEATURE_SOURCE_LAYER_ID != 0 {
        // SAFETY: Caller promises native string storage is valid.
        Some(unsafe { support::string::copy_string_view(raw.source_layer_id) }?)
    } else {
        None
    };
    let state = if raw.fields & sys::MLN_QUERIED_FEATURE_STATE != 0 && !raw.state.is_null() {
        // SAFETY: Caller promises state storage is valid.
        Some(unsafe { JsonValue::from_native(&*raw.state) }?)
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

pub(crate) fn copy_feature_extension_result(
    result: FeatureExtensionResultHandle,
) -> Result<FeatureExtensionResult> {
    let mut info = sys::mln_feature_extension_result_info {
        size: mem::size_of::<sys::mln_feature_extension_result_info>() as u32,
        type_: 0,
        data: sys::mln_feature_extension_result_info__bindgen_ty_1 { value: ptr::null() },
    };
    // SAFETY: result is live and info points to initialized writable storage.
    support::check(unsafe { sys::mln_feature_extension_result_get(result.as_ptr(), &mut info) })?;
    match info.type_ {
        sys::MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE => {
            // SAFETY: Active union member is selected by type_. Native returned
            // a value pointer valid until result drops; this copies it now.
            let value = unsafe { info.data.value };
            if value.is_null() {
                return Err(crate::Error::invalid_argument(
                    "feature extension value result must not be null",
                ));
            }
            // SAFETY: value was checked non-null and is result-owned.
            Ok(FeatureExtensionResult::Value(unsafe {
                JsonValue::from_native(&*value)
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
                copied.push(unsafe { Feature::from_native(feature, 1) }?);
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
        return Err(crate::Error::invalid_argument(format!(
            "{context} pointer must not be null when length is nonzero"
        )));
    }
    // SAFETY: ptr is non-null and caller/native reports len initialized entries.
    Ok(unsafe { std::slice::from_raw_parts(ptr, len) })
}

pub(crate) fn empty_feature() -> sys::mln_feature {
    sys::mln_feature {
        size: mem::size_of::<sys::mln_feature>() as u32,
        geometry: ptr::null(),
        properties: ptr::null(),
        property_count: 0,
        identifier_type: sys::MLN_FEATURE_IDENTIFIER_TYPE_NULL,
        identifier: sys::mln_feature__bindgen_ty_1 { uint_value: 0 },
    }
}

impl super::RenderSessionHandle {
    /// Sets per-feature state on a render source for this session.
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector,
        state: &JsonValue,
    ) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let selector = selector.to_native();
        let state = state.try_to_native()?;
        // SAFETY: session is live. selector and state own all call-scoped
        // descriptor storage until the native call returns.
        support::check(unsafe {
            sys::mln_render_session_set_feature_state(session, selector.as_ptr(), state.as_ptr())
        })
    }

    /// Copies per-feature state from a render source in this session.
    pub fn get_feature_state(&self, selector: &FeatureStateSelector) -> Result<JsonValue> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let selector = selector.to_native();
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: session is live, selector owns call-scoped storage, and out
        // is a null-initialized out-pointer owned by this call.
        support::check(unsafe {
            sys::mln_render_session_get_feature_state(session, selector.as_ptr(), out.as_mut_ptr())
        })?;
        Ok(crate::map::json_snapshot(out.into_option())?
            .unwrap_or_else(|| JsonValue::Object(Vec::new())))
    }

    /// Copies per-feature state from a render source in this session.
    pub fn feature_state(&self, selector: &FeatureStateSelector) -> Result<JsonValue> {
        self.get_feature_state(selector)
    }

    /// Removes per-feature state selected for this session.
    pub fn remove_feature_state(&self, selector: &FeatureStateSelector) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let selector = selector.to_native();
        // SAFETY: session is live and selector owns all call-scoped storage
        // until the native call returns.
        support::check(unsafe {
            sys::mln_render_session_remove_feature_state(session, selector.as_ptr())
        })
    }

    /// Queries rendered features from the latest render session state.
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<Vec<QueriedFeature>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let geometry = geometry.to_native();
        let options = options
            .map(RenderedFeatureQueryOptions::to_native)
            .transpose()?;
        let mut out = support::ptr::OutPtr::<sys::mln_feature_query_result>::new();
        // SAFETY: session is live; geometry and options retain all borrowed
        // descriptor storage for the call; out is a null-initialized owned
        // out-pointer.
        support::check(unsafe {
            sys::mln_render_session_query_rendered_features(
                session,
                geometry.as_ptr(),
                options
                    .as_ref()
                    .map_or(ptr::null(), NativeRenderedFeatureQueryOptions::as_ptr),
                out.as_mut_ptr(),
            )
        })?;
        copy_feature_query_result(FeatureQueryResultHandle::new(
            out.into_non_null("mln_feature_query_result")?,
        ))
    }

    /// Queries source features from the latest render session state.
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<Vec<QueriedFeature>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let options = options
            .map(SourceFeatureQueryOptions::to_native)
            .transpose()?;
        let mut out = support::ptr::OutPtr::<sys::mln_feature_query_result>::new();
        // SAFETY: session is live; source_id and options retain all borrowed
        // descriptor storage for the call; out is a null-initialized owned
        // out-pointer.
        support::check(unsafe {
            sys::mln_render_session_query_source_features(
                session,
                source_id.raw(),
                options
                    .as_ref()
                    .map_or(ptr::null(), NativeSourceFeatureQueryOptions::as_ptr),
                out.as_mut_ptr(),
            )
        })?;
        copy_feature_query_result(FeatureQueryResultHandle::new(
            out.into_non_null("mln_feature_query_result")?,
        ))
    }

    /// Queries a feature extension from the latest render session state.
    pub fn query_feature_extension(
        &self,
        source_id: &str,
        feature: &Feature,
        extension: &str,
        extension_field: &str,
        arguments: Option<&JsonValue>,
    ) -> Result<FeatureExtensionResult> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let extension = support::string::string_view(extension);
        let extension_field = support::string::string_view(extension_field);
        let feature = feature.try_to_native(0)?;
        if let Some(arguments) = arguments
            && !matches!(arguments, JsonValue::Object(_))
        {
            return Err(crate::Error::invalid_argument(
                "feature extension arguments must be a JSON object",
            ));
        }
        let arguments = arguments.map(JsonValue::try_to_native).transpose()?;
        let mut out = support::ptr::OutPtr::<sys::mln_feature_extension_result>::new();
        // SAFETY: session is live; all string, feature, and optional JSON
        // descriptors retain borrowed storage for the call; out is a
        // null-initialized owned out-pointer.
        support::check(unsafe {
            sys::mln_render_session_query_feature_extensions(
                session,
                source_id.raw(),
                feature.as_ptr(),
                extension.raw(),
                extension_field.raw(),
                arguments
                    .as_ref()
                    .map_or(ptr::null(), crate::json::NativeJsonValue::as_ptr),
                out.as_mut_ptr(),
            )
        })?;
        copy_feature_extension_result(FeatureExtensionResultHandle::new(
            out.into_non_null("mln_feature_extension_result")?,
        ))
    }
}
